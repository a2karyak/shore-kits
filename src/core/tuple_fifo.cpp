#include "core/tuple_fifo.h"


ENTER_NAMESPACE(qpipe);

static const int PAGE_SIZE = 4096;



// use 3/9 for "good" performance
#define READ_PREFETCH 3
#define WRITE_PREFETCH 9

inline
void touch(void const*) {
#if 0
    assert(PAGE_SIZE/CACHE_LINE % 8 == 0);
    static int const CACHE_LINE = 64/sizeof(long);
    long volatile *bp = (long*) page;
    long *be = PAGE_SIZE/sizeof(long) + (long*) page;
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
static stats_list file_stats;
static size_t total_prefetches = 0;

/*
 * Creates a new (temporary) file in the BDB bufferpool for the FIFO
 * to use as a backing store. The file will be deleted automatically
 * when the FIFO is destroyed.
 */
void tuple_fifo::init() {
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

stats_list tuple_fifo::get_stats() {
    critical_section_t cs(open_fifo_mutex);
    return file_stats;
}

size_t tuple_fifo::prefetch_count() {
    return total_prefetches;
}
void tuple_fifo::clear_stats() {
    critical_section_t cs(open_fifo_mutex);
    file_stats.clear();
    total_prefetches = 0;
}

/**
 * @brief Potentially makes a page write visible to the reader.
 *
 * A flush occurs if the page is full or a non-empty page is forced.
 */
void tuple_fifo::_flush(bool force) {
    // flush possible?
    if(!_write_page || _write_page->empty())
        return;
    // flush desirable?
    if( !(force || _write_page->full()) )
        return;

    // * * * BEGIN CRITICAL SECTION * * *
    critical_section_t cs(_lock);
    _termination_check();

    _pages.push_back(_write_page.release());

    // notify the reader?
    if(_available_reads() >= _threshold)
        thread_cond_signal(_reader_notify);
    
    // * * * END CRITICAL SECTION * * *
}

//#define USE_PREFETCH

void tuple_fifo::ensure_write_ready() {
    if(check_write_ready()) {
#ifdef USE_PREFETCH
        // issue a prefetch
        size_t count = _write_page->end()->data - _write_page->begin()->data;
        __builtin_prefetch(_prefetch_page->begin()->data + count, 1);
#endif
#if WRITE_PREFETCH != 0
        __builtin_prefetch(_write_page->end()->data + WRITE_PREFETCH*64);
        _prefetch_count++;
#endif
        return;
    }

    // * * * BEGIN CRITICAL SECTION * * *
    critical_section_t cs(_lock);
    _termination_check();
    
    // Wait for space to open up? Once we've slept because of empty,
    // space for _threshold pages must be available before we are
    // willing to try again
    for(size_t threshold=1; _available_writes() < threshold; threshold = _threshold) {
        // sleep until something important changes
        thread_cond_wait(_writer_notify, _lock);
        _termination_check();
    }
    
    // allocate the page (_flush() increments the counter)
    _write_page = page::alloc(tuple_size());
    
    // * * * END CRITICAL SECTION * * *
}

/*
 * @brief Potentially makes a page read visible to the writer.
 *
 * A purge actually occurs if the page has been fully processed or was stolen
 */
bool tuple_fifo::_purge(bool stolen) {
    // always called afer ensure_read_ready(), so should never be NULL
    assert(_read_page);
    
    // purge?
    return stolen || _read_iterator == _read_page->end();
}

bool tuple_fifo::_attempt_tuple_read() {
    // Already armed?
    if(_read_armed)
        return true;

    // do we have a page at all?
    if(!_read_page)
        return false;

    // Anything left on the page?
    ++_read_iterator;
    if(_read_iterator == _read_page->end()) {
        _read_page.done();
        return false;
    }

    // good to go!
#if READ_PREFETCH != 0
    __builtin_prefetch(_read_iterator->data + READ_PREFETCH*64);
    _prefetch_count++;
#endif
    _read_armed = true;
    return true;
}

// true means a page was read
bool tuple_fifo::_attempt_page_read(bool block) {
    
    // * * * BEGIN CRITICAL SECTION * * *
    critical_section_t cs(_lock);
    _termination_check();

    
    // wait for pages to arrive? Once we've slept because of empty,
    // _threshold pages must be available before we are willing to try
    // again (unless send_eof() is called)
    int available_reads = _available_reads();
    for(int threshold=1; available_reads < threshold; threshold = _threshold) {
        // eof?
        if(_done_writing) {
            if(available_reads == 0)
                return false;
            
            // no point to waiting for more pages...
            break;
        }
        
        // must block to get a page; allowed to?
        if(!block)
            return false;
        
        // sleep until something important changes
        thread_cond_wait(_reader_notify, _lock);
        _termination_check();
        available_reads = _available_reads();
    }
    
    // allocate the page
    _read_page = _pages.front();
    _pages.pop_front();
    assert(_read_page);
    _read_iterator = _read_page->begin();
    _read_armed = true;
    
    // notify the writer?
    if(_available_writes() >= _threshold)
        thread_cond_signal(_writer_notify);

    // * * * END CRITICAL SECTION * * *
    return true;
}
 

bool tuple_fifo::send_eof() {
    _flush(true);
    
    // * * * BEGIN CRITICAL SECTION * * *
    critical_section_t cs(_lock);
    if(_terminated)
        return false;

    // make sure the reader wakes up to see the EOF
    _done_writing = true;
    thread_cond_signal(_reader_notify);
    
    // * * * END CRITICAL SECTION * * *
    return true;
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
