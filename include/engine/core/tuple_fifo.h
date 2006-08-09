/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef __TUPLE_FIFO_H
#define __TUPLE_FIFO_H

#include "engine/core/tuple.h"
#include "engine/core/exception.h"
#include "db_cxx.h"

// include me last!
#include "engine/namespace.h"

template<>
inline void guard<DbMpoolFile>::action(DbMpoolFile* ptr) {
    // sync so the darn thing realizes it doesn't have to write out
    // those "dirty" pages
    //    ptr->sync();
    ptr->set_priority(DB_PRIORITY_VERY_LOW);
    ptr->close(0);
}


/**
 *  @brief Thread-safe tuple buffer. This class allows one thread to
 *  safely pass tuples to another. The producer will fill a page of
 *  tuples before handing it to the consumer.
 *
 *  This implementation currently uses an internal allocator to
 *  allocate a new page every time the current page is filled and
 *  handed to the consumer.
 */
class tuple_fifo : public page_pool {

private:

    guard<DbMpoolFile> _pool;
    DbEnv* _dbenv;
    size_t _tuple_size;
    size_t _capacity;
    size_t _threshold;
    size_t _page_size;

    guard<page> _read_page;
    page::iterator _read_iterator;
    bool _read_armed;
    guard<page> _write_page;

    // used to communicate between reader and writer
    volatile db_pgno_t _read_pnum;
    volatile db_pgno_t _write_pnum;
    volatile bool _done_writing;
    volatile bool _terminated;

    // synch vars
    pthread_mutex_t _lock;
    pthread_cond_t _reader_notify;
    pthread_cond_t _writer_notify;


public:

    /**
     *  @brief Construct a tuple FIFO that holds tuples of the
     *  specified size.
     *
     *  @param tuple_size The size of the tuples stored in this
     *  tuple_fifo.
     *
     *  @param num_pages We would like to allocate a new page whenever
     *  we fill up the current page. If the buffer currently contains
     *  this many pages, our insert operations will block rather than
     *  allocate new pages.
     *
     *  @param page_size The size of the pages used in our buffer.
     */

    tuple_fifo(size_t tuple_size,
               DbEnv* env,
               size_t capacity=DEFAULT_BUFFER_PAGES,
               size_t threshold=64,
               size_t page_size=get_default_page_size())
        : page_pool(page_size), _dbenv(env),
          _tuple_size(tuple_size), _capacity(capacity), _threshold(threshold),
          _page_size(page_size), _read_armed(false),
          _read_pnum(1), _write_pnum(1),
          _done_writing(false), _terminated(false),
          _lock(DEFAULT_MUTEX_INITIALIZER),
          _reader_notify(DEFAULT_COND_INITIALIZER),
          _writer_notify(DEFAULT_COND_INITIALIZER)
    {
        init();
    }
    
    // requird by page_pool
    virtual void* alloc();
    virtual void free(void* page);

    DbEnv* dbenv() {
        return _dbenv;
    }
    
    size_t tuple_size() const {
        return _tuple_size;
    }


    size_t page_size() const {
        return _page_size;
    }


    /**
     *  @brief This function can only be called by the
     *  producer. Insert a tuple into this buffer. If the buffer is
     *  full (if it already has allocated the maximum number of
     *  pages), wait for the consumer to read. If the current page is
     *  filled and the buffer is not full, allocate a new page.
     *
     *  @param tuple The tuple to insert. On successful insert, the
     *  data that this tuple points to will be copied into this page.
     *
     *  @return true on successful insert. false if the buffer was
     *  terminated by the consumer.
     *
     *  @throw Can throw exceptions on error.
     */

    void append(const tuple_t &tuple) {
        ensure_write_ready();
        _write_page->append_tuple(tuple);
    }
    
    
    /**
     *  @brief This function can only be called by the
     *  producer. Allocate space for a new tuple in this buffer and
     *  set 'tuple' to point to it. The tuple size stored in the
     *  buffer must match the size of 'tuple'. This function blocks if
     *  the buffer is full.
     *
     *  If this function returns success, the caller may invoke
     *  tuple.assign() to copy data into the allocated space. This
     *  should be done before the next put_tuple() or alloc_tuple()
     *  operation to prevent uninitialized data from being fed to the
     *  consumer of this buffer.
     */

    tuple_t allocate() {
	ensure_write_ready();
        return _write_page->allocate_tuple();
    };
    

    bool get_tuple(tuple_t &tuple) {
        if(!ensure_read_ready())
            return false;

        tuple = *_read_iterator;
        _read_armed = false;
        return true;
    }


    /**
     *  @brief This function can only be called by the
     *  consumer. Retrieve a page of tuples from the buffer in one
     *  operation. The buffer gives up ownership of the page and will
     *  not access (or free) it afterward.
     *
     *  WARNING: Do not mix calls to get_page() and get_tuple() unless
     *  the caller if prepared to deal with duplicate tuples. In other
     *  words, the caller must be willing to process some unknown
     *  number of tuples twice.
     *
     *  @param pagep If a page is retrieved, it is assigned to this
     *  tuple_page_t pointer. If the caller is mixing calls to
     *  get_page() and get_tuple(), some of the tuples on the returned
     *  page may have been returned by previous calls to get_tuple()).
     *
     *  @return NULL if the buffer is empty and the producer has
     *  invoked send_eof() on the buffer.
     *
     *  @throw BufferTerminatedException if the producer has
     *  terminated the buffer.
     */

    page* get_page() {
        if(!ensure_read_ready())
            return NULL;

        // no mixing allowed!
        assert(_read_iterator == _read_page->begin());
        _purge(true);
        _read_armed = false;
        return _read_page.release();
    }


    /**
     * @brief ensures that the FIFO is ready for writing.
     *
     * When this function returns at least one tuple may be written
     * without blocking.
     */
    void ensure_write_ready();

    /**
     * @brief ensures that the FIFO is ready for reading.
     *
     * The call will block until a tuple becomes available for
     * reading, or EOF is encountered
     *
     * @return false if EOF
     */
    bool ensure_read_ready() {
        // blocking attempt => only returns false if EOF
        return _attempt_tuple_read() || _attempt_page_read(true);
    }


    /**
     * @brief Determines whether the FIFO is ready for writing (non-blocking).
     */
    bool check_write_ready() {
        assert(!_done_writing);
        _flush(false);
        return _write_page != NULL;
    }

    /**
     * @brief Determines whether the FIFO is ready for reading (non-blocking)
     *
     * @return true if there are tuples ready to read, or if EOF
     */
    bool check_read_ready() {
        // can we advance an existing iterator or grab a new page
        // without blocking?
        return _attempt_tuple_read() || _attempt_page_read(false) || _done_writing;

        // non-blocking attempt => false could mean either EOF (return
        // true) or empty (return false)
    }
    
    /**
     * @brief Closes the buffer immediately (and abnormally).
     *
     * If successful, the other end will be notified through an
     * exception at some point in the future and will be responsible
     * for deleting the FIFO. The caller must not access the FIFO
     * again if terminate() returns true.
     *
     * @return true if termination succeeded. false means the buffer
     * was already terminated or at EOF.
     * 
     */
    bool terminate();


    /**
     * @brief Signals the reader that the writer has finished
     * producing tuples.
     *
     * The FIFO will close normally once the last tuple has been read;
     * the reader is responsible for deleting it. The writer must not
     * access the FIFO again if send_eof() returns true.
     *
     */
    bool send_eof();
    
    
    
    /**
     *  @brief Check if we have consumed all tuples from this
     *  buffer.
     *
     *  @return Returns true only if the producer has sent EOF and we
     *  have read every tuple of every page in this buffer.
     */
    bool eof() {
        _termination_check();
        return !_read_page && !_available_reads() && _done_writing;
    }


private:
    size_t _available_writes() {
        return _capacity - (_write_pnum - _read_pnum);
    }

    size_t _available_reads() {
        return _write_pnum - _read_pnum;
    }

    void _termination_check() {
        if(_terminated)
            EXCEPTION(TerminatedBufferException, "Buffer closed unexpectedly");
    }


    /**
     * @brief flushes the current write page if it is full (or if forced)
     */
    void _flush(bool force);

    bool _purge(bool stolen);

    void init();

    void _unpin(page* p, bool keep);

    // attempts to read a tuple from an existing page
    bool _attempt_tuple_read();
    // attempts to read a new page
    bool _attempt_page_read(bool block);

    page* _pin(db_pgno_t pnum);
};



#include "engine/namespace.h"



template<>
inline void guard<qpipe::tuple_fifo>::action(qpipe::tuple_fifo* ptr) {
    if(!ptr->terminate()) {
        printf(">>> Deleting a fifo\n");
        delete ptr;
    }
}


#endif
