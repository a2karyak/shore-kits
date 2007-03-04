/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef __TUPLE_FIFO_H
#define __TUPLE_FIFO_H

#include "core/tuple.h"
#include <vector>
#include <list>
#include <ucontext.h>

ENTER_NAMESPACE(qpipe);


DEFINE_EXCEPTION(TerminatedBufferException);



/**
 *  @brief Thread-safe tuple buffer. This class allows one thread to
 *  safely pass tuples to another. The producer will fill a page of
 *  tuples before handing it to the consumer.
 *
 *  This implementation currently uses an internal allocator to
 *  allocate a new page every time the current page is filled and
 *  handed to the consumer.
 */
class tuple_fifo {

private:
    typedef std::list<page*> page_list;
    page_list _pages;
    page_list _free_pages;
    
    size_t _tuple_size;
    size_t _capacity;
    size_t _threshold;
    size_t _page_size;
    size_t _prefetch_count;
    size_t _curr_pages;
    size_t _num_inserted;
    size_t _num_removed;
    size_t _num_waits_on_insert;
    size_t _num_waits_on_remove;
    char*  _read_end;

    guard<page> _read_page;
    page::iterator _read_iterator;
    guard<page> _write_page;

    // used to communicate between reader and writer
    volatile bool _done_writing;
    volatile bool _terminated;

    // synch vars
    pthread_mutex_t _lock;
    pthread_cond_t _reader_notify;
    pthread_cond_t _writer_notify;

    // debug vars
    pthread_t _reader_tid;
    pthread_t _writer_tid;
    
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
               size_t capacity=DEFAULT_BUFFER_PAGES,
               size_t threshold=64,
               size_t page_size=get_default_page_size())
        : 
          _tuple_size(tuple_size),
          _capacity(capacity),
          _threshold(threshold),
          _page_size(page_size),
          _prefetch_count(0),
          _curr_pages(0),
          _num_inserted(0),
          _num_removed(0),
          _num_waits_on_insert(0),
          _num_waits_on_remove(0),
          _done_writing(false),
          _terminated(false),
          _lock(thread_mutex_create()),
          _reader_notify(thread_cond_create()),
          _writer_notify(thread_cond_create()),
	  _reader_tid(0),
          _writer_tid(0)
    {
        init();
    }

    ~tuple_fifo() {
        destroy();
    }


    /* Global tuple_fifo statistics */
    static int open_fifos();
    static size_t prefetch_count();
    static void clear_stats();
    static void trace_stats();


    size_t tuple_size() const {
        return _tuple_size;
    }


    size_t page_size() const {
        return _page_size;
    }


    void writer_init();


    /**
     *  @brief Only the producer may call this method. Insert a tuple
     *  into this buffer. If the buffer is full (if it already has
     *  allocated the maximum number of pages), wait for the consumer
     *  to read. If the current page is filled and the buffer is not
     *  full, allocate a new page.
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
        _num_inserted++;
        _write_page->append_tuple(tuple);
    }
    
    
    /**
     *  @brief Only the producer may call this method. Allocate space
     *  for a new tuple in this buffer and set the returned tuple to
     *  point to it. The caller may then assign to this tuple to
     *  assign to the buffer.
     *
     *  If this function returns success, the caller may invoke
     *  tuple.assign() to copy data into the allocated space. This
     *  should be done before the next put_tuple() or alloc_tuple()
     *  operation to prevent uninitialized data from being fed to the
     *  consumer of this buffer.
     */

    tuple_t allocate() {
        ensure_write_ready();
        _num_inserted++;
        return _write_page->allocate_tuple();
    };
    

    bool get_tuple(tuple_t &tuple) {
        if(!ensure_read_ready())
            return false;
       
        tuple = *_read_iterator++;
        _num_removed++;
        return true;
    }


    /**
     *  @brief Only the consumer may call this method. Retrieve a page
     *  of tuples from the buffer in one operation. The buffer gives
     *  up ownership of the page and will not access (or free) it
     *  afterward.
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

    page* get_page();


    /**
     * @brief ensures that the FIFO is ready for writing.
     *
     * When this function returns at least one tuple may be written
     * without blocking.
     */

    void ensure_write_ready() {
        if(_write_page->full())
            _flush_write_page(false);
    }
        

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
	return (_read_iterator->data != _read_end) || _get_read_page();
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
        return !ensure_read_ready();
    }


private:

    size_t _available_writes() {
        return _capacity - _available_reads();
    }

    size_t _available_reads() {
        return _curr_pages;
    }

    void _termination_check() {
        if(_terminated)
            THROW1(TerminatedBufferException, "Buffer closed unexpectedly");
    }

    void _set_read_page(page* p) {
	_read_page = p;
	_read_iterator = _read_page->begin();
	_read_end = _read_page->end()->data;
    }

    void init();
    void destroy();

    // attempts to read a new page
    bool _get_read_page();
    void _flush_write_page(bool done_writing);
    
    void wait_for_reader();
    void ensure_reader_running();
    
    void wait_for_writer();
    void ensure_writer_running();
    
};



EXIT_NAMESPACE(qpipe);



template<>
inline void guard<qpipe::tuple_fifo>::action(qpipe::tuple_fifo* ptr) {
    if(!ptr->terminate()) {
        delete ptr;
    }
}


#endif
