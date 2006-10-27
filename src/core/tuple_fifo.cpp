#include "core/tuple_fifo.h"


ENTER_NAMESPACE(qpipe);

static const int PAGE_SIZE = 4096;



// use 3/9 for "good" performance
#define READ_PREFETCH 3
#define WRITE_PREFETCH 9

inline
void touch(void const* page) {
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

    DbMpoolFile* pool;
    dbenv()->memp_fcreate(&pool, 0);
    _pool = pool;
    _pool->open(NULL, DB_CREATE|DB_DIRECT, 0644, PAGE_SIZE);
}

void tuple_fifo::destroy() {
    // stats
    critical_section_t cs(open_fifo_mutex);
    
    // store the stats for later...
    file_stats.push_back(_pool->get_stats());
    DB_MPOOL_FSTAT &stat = file_stats.back();
    total_prefetches += _prefetch_count;

    // canonicalize the name so we don't have dangling pointers
    stat.file_name = "FIFO";
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

/*
 * Allocates a new page at the end of the FIFO's file and pins it in
 * the BDB buffer pool
 */
void* tuple_fifo::alloc() {
    db_pgno_t pnum;
    void* page;
    _pool->get(&pnum, DB_MPOOL_NEW, &page);
    return page;
}

/*
 * Discards a page from the buffer pool. It is marked as clean (no
 * write-back needed) and flagged as a good candidate for eviction.
 */
void tuple_fifo::free(void* p) {
    _unpin(static_cast<page*>(p), false);
}

/*
 * Unpins a page from the buffer pool. If marked to keep, it will be
 * flagged as dirty; otherwise it will be effectively deleted by
 * marking it as clean and evictable. If the page was already written
 * to disk it will remain there until the FIFO is destroyed and the
 * file is deleted.
 */
void tuple_fifo::_unpin(page* p, bool keep) {
    int flags = keep? DB_MPOOL_DIRTY : DB_MPOOL_TRASH;
    _pool->put(p, flags);
}

/*
 * Pins a page in the buffer pool, retrieving it from disk as necessary.
 */
page* tuple_fifo::_pin(db_pgno_t pnum) {
    union {
        void* v;
        page* p;
    } pun;

    _pool->get(&pnum, 0, &pun.v);
    return pun.p;
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

    // flush!
    _unpin(_write_page.release(), true);

    // * * * BEGIN CRITICAL SECTION * * *
    critical_section_t cs(_lock);
    _termination_check();

    // update state 
    _write_pnum++;

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
#ifdef USE_PREFETCH
    do {
        _write_page = _prefetch_page;
        _prefetch_page = page::alloc(tuple_size(), this);
    } while(_write_page == NULL);
#else
    _write_page = page::alloc(tuple_size(), this);
#endif
    touch(&_write_page);
    
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
    if( !(stolen || _read_iterator == _read_page->end()) )
        return false;

    // * * * BEGIN CRITICAL SECTION * * *
    critical_section_t cs(_lock);
    _termination_check();

    // update state 
    _read_pnum++;
    
    // notify the writer?
    if(_available_writes() >= _threshold)
        thread_cond_signal(_writer_notify);

    // * * * END CRITICAL SECTION * * *
    return true;
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
    if(_purge(false)) {
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
    
    // allocate the page (_purge() increments the counter)
    _read_page = _pin(_read_pnum);
    touch(&_write_page);
    
    assert(_read_page);
    _read_iterator = _read_page->begin();
    _read_armed = true;

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
