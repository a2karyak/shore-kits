/** -*- mode:C++; c-basic-offset:4 -*- */
#include "core/tuple_fifo.h"
#include <sys/mman.h>


ENTER_NAMESPACE(qpipe);


bool tuple_fifo::DEBUG_CTX_SWITCH = false;
#define DEBUG_MMAP false



#define MMAP_FAILURE ((char*) -1)

static void unmap(char* start, char* end) {
    // anything to do?
    if(start == end)
	return;
    
    int result = munmap(start, end - start);
    if(result != 0)
	TRACE(TRACE_ALWAYS, "munmap() failed on %p - %p: %s\n",
	      start, end, errno_to_str(errno).data());
    else
        if (DEBUG_MMAP)
            TRACE(TRACE_ALWAYS, "unmapped %p - %p\n", start, end);
}

void* mmap_page_pool::alloc() {
    critical_section_t cs(_mutex);
    if(_available_start == _available_end) {
	// try to recycle recently freed memory (save a munmap+mmap pair)
	if(_free_start != _free_end) {
	    _available_start = _free_start;
	    _available_end = _free_end;
	    _free_start = _free_end = NULL;
	}
	
	else {
	    // allocate in chunks of 1008k, which becomes exactly 1M
	    // with red zones. Anything smaller gets rounded up
	    // anyway. See "man mmap" for the gory details.
	    size_t len = 1008*1024;
	    _available_start = (char*) mmap(0, len, PROT_READ|PROT_WRITE,
					    MAP_PRIVATE|MAP_ANON, -1, 0);
	    if(_available_start == MMAP_FAILURE)
		THROW2(BadAlloc, "mmap() failed: %s", errno_to_str(errno).data());
	    _available_end = _available_start + len;
            if (DEBUG_MMAP)
                TRACE(TRACE_ALWAYS, "mapped %p - %p\n", _available_start, _available_end);
	}
    }

    // pick off the next available page
    char* next = _available_start;
    _available_start += page_size();
    ++_outstanding;
    return next;
}

void mmap_page_pool::free(void* ptr) {
    critical_section_t cs(_mutex);

    // accumulate the largest sequence possible before actually
    // calling munmap()
    char* data = (char*) ptr;

    // do we have to start a new sequence?
    if(_free_end != data) {
	// was there a previous sequence?
	if(_free_end != NULL)
	    unmap(_free_start, _free_end);

	// start a new sequence
	_free_start = _free_end = data;
    }
    
    // add this page to the current sequence (which may have been length 0)
    _free_end += page_size();

    // scary, but it works. Just don't touch me again!
    if(--_outstanding == 0) {
	cs.exit();
	delete this;
    }
}

mmap_page_pool::~mmap_page_pool() {
    // remember to let go of any stragglers!
    unmap(_available_start, _available_end);
    unmap(_free_start, _free_end);
}

/* The pool that manages the sentinel page. Only one page will ever be
 * "created", and it must not actually go away when freed.
 */
struct sentinel_page_pool : page_pool {
    // ensure enough space for one 1-byte tuple (so the page is empty)
    char _data[sizeof(qpipe::page)];
    sentinel_page_pool()
        : page_pool(sizeof(qpipe::page))
    {
        /* We need a really special page with the following properties:
         * - read sentinel: p->begin() == p->end()
         * - write sentinel: p->full()
         * - flush sentinel: p->empty()
         */
        //        page* p = page::alloc(1, this);
        //        assert(p->begin() == p->end() && p->full() && p->empty());
    }
    virtual void* alloc() {
        return _data;
    }
    virtual void free(void*) {
        // do nothing.
    }
};

/* A sentinel page used to avoid corner cases in page reads. It is used
 * - Before the first call to advance_reader()
 * - After any call to read_page()
 * - During initialization of the write page
 *
 * A tuple size of 1 is almost certainly illegal for any FIFO
 * instance, but since it is always an error for the user to call
 * read() or read_page() without first calling advance_reader(), we
 * don't care.
 */
static sentinel_page_pool SENTINEL_POOL;
static qpipe::page* SENTINEL_PAGE = qpipe::page::alloc(1, &SENTINEL_POOL);
//static const int QPIPE_PAGE_SIZE = 4096;


// use 3/9 for "good" performance
#define READ_PREFETCH 3
#define WRITE_PREFETCH 9

inline
void touch(void const*) {
#if 0
    assert(QPIPE_PAGE_SIZE/CACHE_LINE % 8 == 0);
    static int const CACHE_LINE = 64/sizeof(long);
    long volatile *bp = (long*) page;
    long *be = QPIPE_PAGE_SIZE/sizeof(long) + (long*) page;
    for(; bp < be; bp+=CACHE_LINE*8) {
        bp[0*CACHE_LINE];
        bp[1*CACHE_LINE];
        bp[2*CACHE_LINE];
        bp[3*CACHE_LINE];
        bp[4*CACHE_LINE];
        bp[5*CACHE_LINE];
        bp[6*CACHE_LINE];
        bp[7*CACHE_LINE];
    } 
#endif
}

struct strlt {
    bool operator()(char* const a, char* const b) {
        return strcmp(a, b) < 0;
    }
};

pthread_mutex_t open_fifo_mutex = thread_mutex_create();
static int open_fifo_count = 0;

static size_t total_prefetches = 0;

/*
 * Creates a new (temporary) file in the BDB bufferpool for the FIFO
 * to use as a backing store. The file will be deleted automatically
 * when the FIFO is destroyed.
 */
void tuple_fifo::init() {
    // prepare for reading
    _set_read_page(SENTINEL_PAGE);

    // prepare for writing
    _write_page = SENTINEL_PAGE;
    
    // stats
    critical_section_t cs(open_fifo_mutex);
    open_fifo_count++;
    cs.exit();
}

void tuple_fifo::destroy() {
    // stats
    critical_section_t cs(open_fifo_mutex);
    
    // store the stats for later...
    total_prefetches += _prefetch_count;

    // canonicalize the name so we don't have dangling pointers
    open_fifo_count--;
}

int tuple_fifo::open_fifos() {
    return open_fifo_count;
}

size_t tuple_fifo::prefetch_count() {
    return total_prefetches;
}
void tuple_fifo::clear_stats() {
    critical_section_t cs(open_fifo_mutex);
    total_prefetches = 0;
}

page* tuple_fifo::get_page() {
    if(!ensure_read_ready())
        return NULL;

    // no partial pages allowed!
    assert(_read_iterator == _read_page->begin());

    // hand off the page and replace it with the sentinel
    page* result = _read_page.release();
    _set_read_page(SENTINEL_PAGE);
    return result;
}

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
        fprintf(stderr, "Fifo %p sending EOF\n", this);
        _done_writing = true;
        _write_page.done();
    }
    else
        _write_page = page::alloc(tuple_size(), _pool);

    // notify the reader?
    if(_available_reads() >= _threshold || done_writing) {
        //        fprintf(stderr, "Fifo %p notifying reader\n", this);
        ensure_reader_running();
    }
    
    // * * * END CRITICAL SECTION * * *
}

bool tuple_fifo::_get_read_page() {
    // * * * BEGIN CRITICAL SECTION * * *
    critical_section_t cs(_lock);
    _termination_check();


    // wait for pages to arrive? Once we've slept because of empty,
    // _threshold pages must be available before we are willing to try
    // again (unless send_eof() is called)
    for(size_t t=1; !_done_writing && _available_reads() < t; t = _threshold) {
        // sleep until something important changes
        //        fprintf(stderr, "Fifo %p sleeping on read\n", this);
        wait_for_writer();
        _termination_check();
    }

    if(_available_reads() == 0) {
        assert(_done_writing);
        return false;
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
    return true;
}


bool tuple_fifo::send_eof() {
    _pool->release();
    try {

        // make sure not to drop a partial page
        _flush_write_page(true);

        //        DEBUG_CTX_SWITCH = true;

        if (DEBUG_CTX_SWITCH) {
            TRACE(TRACE_ALWAYS, "Running in context %s\n",
                  thread_get_self()->_ctx->_context_name.data());
            TRACE(TRACE_ALWAYS, "FIFO thinks we are going to switch from %s to %s\n",
                  _write_ctx->_context_name.data(),
                  _read_ctx->_context_name.data());
        }

        /* writer never returns */
        assert(thread_get_self()->_ctx == _write_ctx);
        int swapret = swapcontext(&_write_ctx->_context, &_read_ctx->_context);
        assert(swapret == 0);
        assert(0);

        return true;
    }
    catch(TerminatedBufferException &e) {
        // oops! reader terminated first
        return false;
    }
}


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
 
EXIT_NAMESPACE(qpipe);
