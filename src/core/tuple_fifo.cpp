/** -*- mode:C++; c-basic-offset:4 -*- */

#include "core/tuple_fifo.h"
#include "core/tuple_fifo_directory.h"
#include "util/trace.h"
#include "util/acounter.h"
#include <algorithm>



ENTER_NAMESPACE(qpipe);



/* Prefetch constants */

/* use 3/9 for "good" performance */
#define READ_PREFETCH  3
#define WRITE_PREFETCH 9



/* debugging */
static int TRACE_MASK_BLOCKING = TRACE_COMPONENT_MASK_NONE;



/* Global tuple_fifo statistics */

/* statistics data structures */
static pthread_mutex_t open_fifo_mutex = thread_mutex_create();
static int open_fifo_count = 0;
static int total_fifos_existed = 0;
static int total_fifos_experienced_blocking = 0;
static size_t total_prefetches = 0;



/* statistics methods */

/**
 * @brief Return the number of currently open tuple_fifos. Not
 * synchronized.
 */
int tuple_fifo::open_fifos() {
    return open_fifo_count;
}



/**
 * @brief Return the number of prefetches done so far. Not
 * synchronized.
 */
size_t tuple_fifo::prefetch_count() {
    return total_prefetches;
}



/**
 * @brief Reset global statistics to initial values. This method _is_
 * synchronized.
 */
void tuple_fifo::clear_stats() {
    critical_section_t cs(open_fifo_mutex);
    total_prefetches = 0;
}



/**
 * @brief Dump stats using TRACE.
 */
void tuple_fifo::trace_stats() {
    TRACE(TRACE_ALWAYS, "Fraction of tuple_fifos that experienced blocking = %lf\n",
          (double)total_fifos_experienced_blocking/total_fifos_existed);
}



/* Sentinal page pool. */

/**
 * @brief Every page within a tuple_fifo needs to be part of a
 * page_pool that manages its deletion. The sentinal_page_pool
 * allocates a single global "sentinal page" that is never freed. This
 * page is inserted into every tuple_fifo to reduce the number of
 * corner cases it needs to deal with.
 *
 * The sentinal page does not go away when it is "freed".
 */
struct sentinel_page_pool : page_pool {
    
    /* We will use this single buffer for our global page instance */
    char _data[sizeof(qpipe::page)];

    sentinel_page_pool()
        : page_pool(sizeof(qpipe::page))
    {
    }

    virtual void* alloc() {
        return _data;
    }

    virtual void free(void*) {
        /* do nothing */
    }
};


/* sentinal page data structures */

static sentinel_page_pool SENTINEL_POOL;
static qpipe::page* SENTINEL_PAGE = qpipe::page::alloc(1, &SENTINEL_POOL);
static const int QPIPE_PAGE_SIZE = 4096;



/* definitions of exported methods */

/*
 * Creates a new (temporary) file in the BDB bufferpool for the FIFO
 * to use as a backing store. The file will be deleted automatically
 * when the FIFO is destroyed.
 */
void tuple_fifo::init() {

    /* Check on the tuple_fifo directory. */
    tuple_fifo_directory_t::open_once();

    _reader_tid = pthread_self();

    /* Prepare for reading. */
    _set_read_page(SENTINEL_PAGE);

    /* Prepare for writing. */
    _write_page = SENTINEL_PAGE;
    
    /* Update statistics. */
    critical_section_t cs(open_fifo_mutex);
    open_fifo_count++;
    total_fifos_existed++;
    cs.exit();
}



/**
 * @brief Should be invoked by the writer before writing tuples.
 */
void tuple_fifo::writer_init() {
    /* set up the _writer_tid field to make debugging easier */
    _writer_tid = pthread_self();
}



/**
 * @brief Function object. Wrapper around p->free() that can be passed
 * to std::for_each.
 */
struct free_page {
    void operator()(qpipe::page* p) {
	p->free();
    }
};



/**
 * @brief Deallocate the pags in this tuple_fifo and add our local
 * statistics to global ones.
 */
void tuple_fifo::destroy() {

    std::for_each(_pages.begin(), _pages.end(), free_page());
    std::for_each(_free_pages.begin(), _free_pages.end(), free_page());
	
    /* update stats */
    critical_section_t cs(open_fifo_mutex);
    total_prefetches += _prefetch_count;
    open_fifo_count--;
    if ((_num_waits_on_insert > 0) || (_num_waits_on_remove > 0))
        total_fifos_experienced_blocking++;


    TRACE(TRACE_MASK_BLOCKING & TRACE_ALWAYS,
          "Blocked on insert %.2f\n",
          (double)_num_waits_on_insert/_num_inserted);
    TRACE(TRACE_MASK_BLOCKING & TRACE_ALWAYS,
          "Blocked on remove %.2f\n",
          (double)_num_waits_on_remove/_num_removed);
}



/**
 * @brief Get a page from the tuple_fifo.
 *
 * @return NULL if the tuple_fifo has been closed. A page otherwise.
 */
page* tuple_fifo::get_page(int timeout) {
    if(!ensure_read_ready(timeout))
        return NULL;

    // no partial pages allowed!
    assert(_read_iterator == _read_page->begin());

    // hand off the page and replace it with the sentinel
    page* result = _read_page.release();
    _set_read_page(SENTINEL_PAGE);

    // return page
    _num_removed += result->tuple_count();
    return result;
}



/**
 * @brief Only the producer may call this method. Notify the
 * tuple_fifo that the caller will be inserting no more data.
 *
 * @return True if we successfully send and EOF. False if the fifo has
 * been terminated.
 */
bool tuple_fifo::send_eof() {
    try {
        // make sure not to drop a partial page
        _flush_write_page(true);
        return true;
    }
    catch(TerminatedBufferException &e) {
        // oops! reader terminated first
        return false;
    }
}



/**
 * @brief Both producer and consumer may call this method. This method
 * is used to notify the "other side" that we are terminating
 * abnormally. The other side should receive a
 * TerminatedBufferException on the next tuple_fifo operation.
 *
 * The side which terminates last is responsible for deleting the
 * tuple_fifo.
 *
 * @return True if we successfully terminate. False if the other side
 * has already terminated.
 */
bool tuple_fifo::terminate() {
    
    // * * * BEGIN CRITICAL SECTION * * *
    critical_section_t cs(_lock);


    // did the other end beat me to it? Note: if _done_writing is true
    // the reader is responsible for deleting the buffer; either the
    // the reader doesn't know yet or the writer is making an illegal
    // access. False is the right return value for both cases.
    if(_terminated || _done_writing)
        return false;
    
    // make sure nobody is sleeping (either the reader or writer could
    // be calling this)
    _terminated = true;
    thread_cond_signal(_reader_notify);
    thread_cond_signal(_writer_notify);
    
    // * * * END CRITICAL SECTION * * *
    return true;
}



/* definitions of helper methods */


inline void tuple_fifo::wait_for_reader() {
    _num_waits_on_insert++;
    thread_cond_wait(_writer_notify, _lock);
}

inline void tuple_fifo::ensure_reader_running() {
    thread_cond_signal(_reader_notify);
}

inline bool tuple_fifo::wait_for_writer(int timeout) {
    _num_waits_on_remove++;
    return thread_cond_wait(_reader_notify, _lock, timeout);
}

inline void tuple_fifo::ensure_writer_running() {
    thread_cond_signal(_writer_notify);
}



/**
 * @brief Get a page from the tuple_fifo.
 *
 * @return NULL if the tuple_fifo has been closed. A page otherwise.
 */
void tuple_fifo::_flush_write_page(bool done_writing) {

    // after the call to send_eof() the write page is NULL
    assert(!_done_writing);

    // * * * BEGIN CRITICAL SECTION * * *
    critical_section_t cs(_lock);
    _termination_check();

    // Wait for space to open up? Once we've slept because of empty,
    // space for _threshold pages must be available before we are
    // willing to try again
    for(size_t threshold=1; _available_writes() < threshold; threshold = _threshold) {
        //        fprintf(stderr, "Fifo %p sleeping on write\n", this);
        // sleep until something important changes
        wait_for_reader();
        _termination_check();
    }

    // allocate the page
    if(!_write_page->empty()) {
        _pages.push_back(_write_page.release());
        _curr_pages++;
        _prefetch_count++;
    }

    if(done_writing) {
	//        fprintf(stderr, "Fifo %p sending EOF\n", this);
        _done_writing = true;
        _write_page.done();
    }
    else if(_free_pages.empty())
        _write_page = page::alloc(tuple_size());
    else {
	_write_page = _free_pages.front();
	_free_pages.pop_front();
    }

    // notify the reader?
    if(_available_reads() >= _threshold || done_writing) {
        //        fprintf(stderr, "Fifo %p notifying reader\n", this);
        ensure_reader_running();
    }
    
    // * * * END CRITICAL SECTION * * *
}

int tuple_fifo::_get_read_page(int timeout) {

    // * * * BEGIN CRITICAL SECTION * * *
    critical_section_t cs(_lock);
    _termination_check();

    // free the page so the writer can use it
    if(_read_page != SENTINEL_PAGE) {
	_read_page->clear();
	_free_pages.push_back(_read_page.release());
	_set_read_page(SENTINEL_PAGE);
    }
    
    // wait for pages to arrive? Once we've slept because of empty,
    // _threshold pages must be available before we are willing to try
    // again (unless send_eof() is called)
    if(timeout >= 0) {
    for(size_t t=1; !_done_writing && _available_reads() < t; t = _threshold) {
        // sleep until something important changes
        //        fprintf(stderr, "Fifo %p sleeping on read\n", this);
        if(!wait_for_writer(timeout))
	    break;
	
        _termination_check();
    }
    }
    if(_available_reads() == 0) {
	if(_done_writing)
	    return -1;
	if(timeout != 0)
	    return 0;

	// one of the previous is required to be true!
	unreachable();
    }

    // allocate the page
    _set_read_page(_pages.front());
    _pages.pop_front();
    _curr_pages--;

    // notify the writer?
    if(_available_writes() >= _threshold && !_done_writing) {
        //        fprintf(stderr, "Fifo %p notifying writer\n", this);
        ensure_writer_running();
    }

    // * * * END CRITICAL SECTION * * *
    return 1;
}



int tuple_fifo_generate_id() {
    static acounter_t next_fifo_id;
    return next_fifo_id.fetch_and_inc();
}



EXIT_NAMESPACE(qpipe);
