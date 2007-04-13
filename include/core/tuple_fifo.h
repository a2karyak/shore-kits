/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef __TUPLE_FIFO_H
#define __TUPLE_FIFO_H

#include "core/tuple.h"
#include <cstdio>
#include <vector>
#include <list>
#include <ucontext.h>

ENTER_NAMESPACE(qpipe);


DEFINE_EXCEPTION(TerminatedBufferException);


int tuple_fifo_generate_id();


extern bool FLUSH_TO_DISK_ON_FULL;
extern bool USE_DIRECT_IO;



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

    /* internal datatypes */

    typedef std::list<page*> page_list;

    class tuple_fifo_state_t {
    public:
        enum _tuple_fifo_state_t {
            INVALID = 0,
            IN_MEMORY,
            IN_MEMORY_DONE_WRITING,
            IN_MEMORY_TERMINATED,
            ON_DISK,
            ON_DISK_DONE_WRITING,
            ON_DISK_TERMINATED
        };
        
    private:
        volatile _tuple_fifo_state_t _value;

    public:

        tuple_fifo_state_t ()
            : _value(INVALID)
        {
        }

        static c_str state_to_string(const _tuple_fifo_state_t value) {
            switch(value) {
#define TF_STATE(x) case x: return #x;
                TF_STATE(INVALID);
                TF_STATE(IN_MEMORY);
                TF_STATE(IN_MEMORY_DONE_WRITING);
                TF_STATE(IN_MEMORY_TERMINATED);
                TF_STATE(ON_DISK);
                TF_STATE(ON_DISK_DONE_WRITING);
                TF_STATE(ON_DISK_TERMINATED);
            default:
                assert(0);
            }
        }
        
        c_str to_string() {
            return state_to_string(current());
        }

        _tuple_fifo_state_t current() {
            return _value;
        }
        
        void transition(const _tuple_fifo_state_t next);

private:

        bool _transition_ok(const _tuple_fifo_state_t next);
    };


    /* ID */
    int _fifo_id;

    /* state */
    tuple_fifo_state_t _state;
    volatile bool _is_shared;

    /* page list management */
    page_list _pages;
    page_list _free_pages;
    size_t _page_count;
    size_t _pages_in_memory;
    size_t _available_reads;
    size_t _memory_capacity;
    size_t _threshold;

    /* page file management */
    int    _page_file;
    bool   _free_read_page;
    ssize_t _next_write_page_index;
    ssize_t _next_read_page_index;
    ssize_t _list_head_page_index;
    ssize_t _list_tail_page_index;
    ssize_t _file_head_page_index;
    ssize_t _file_tail_page_index;
    
    /* useful fields to store */
    size_t _tuple_size;
    size_t _page_size;

    /* stats (don't affect correctness) */
    size_t _num_inserted;
    size_t _num_removed;
    size_t _num_waits_on_insert;
    size_t _num_waits_on_remove;

    /* read and write page management */
    char*  _read_end;
    guard<page> _read_page;
    page::iterator _read_iterator;
    guard<page> _write_page;

    /* synch vars */
    pthread_mutex_t _lock;
    pthread_cond_t _reader_notify;
    pthread_cond_t _writer_notify;

    /* debug vars */
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
     *  @param capacity We would like to allocate a new page whenever
     *  we fill up the current page. If the buffer currently contains
     *  this many pages, our insert operations will block rather than
     *  allocate new pages.
     *
     *  @param threshold If the writer sees a full tuple_fifo, it
     *  waits until this many free pages appear before writing
     *  more. If a reader sees this many, it wait until this many full
     *  pages appear or until the writer finishes writing.
     *
     *  @param page_size The size of the pages used in our buffer.
     */
    tuple_fifo(size_t tuple_size,
               size_t capacity=DEFAULT_BUFFER_PAGES,
               size_t threshold=64,
               size_t page_size=get_default_page_size())
        : _fifo_id(tuple_fifo_generate_id()),
          _is_shared(false),
          _page_count(0),
          _pages_in_memory(0),
          _available_reads(0),
          _memory_capacity(capacity),
          _threshold(threshold),
          _page_file(-1),
          _free_read_page(true),
          _next_write_page_index(0), /* ID of current write page */
          _next_read_page_index (0), /* ID of next page to be read */
          _list_head_page_index(-1),
          _list_tail_page_index(-1),
          _file_head_page_index(-1),
          _file_tail_page_index(-1),
          _tuple_size(tuple_size),
          _page_size(page_size),
          _num_inserted(0),
          _num_removed(0),
          _num_waits_on_insert(0),
          _num_waits_on_remove(0),
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
    static int  open_fifos();
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
     *  @throw Can throw TerminatedBufferException if the reader has
     *  terminated the buffer.
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
     *  When this function returns, the caller may invoke
     *  tuple.assign() to copy data into the allocated space. This
     *  should be done before the next put_tuple() or alloc_tuple()
     *  operation to prevent uninitialized data from being fed to the
     *  consumer of this buffer.
     *
     *  @throw Can throw TerminatedBufferException if the consumer has
     *  terminated the buffer.
     */
    tuple_t allocate() {
        ensure_write_ready();
        _num_inserted++;
        return _write_page->allocate_tuple();
    };
    

    /**
     *  @brief Only the consumer may call this method. If the buffer
     *  contains more tuples, set 'tuple' to be the next tuple in the
     *  buffer and return true. If the buffer has been closed and is
     *  empty, return false.
     *
     *  @throw Can throw TerminatedBufferException if the producer has
     *  terminated the buffer.
     */
    bool get_tuple(tuple_t &tuple) {
        if (!ensure_read_ready())
            return false;
        tuple = *_read_iterator++;
        _num_removed++;
        return true;
    }


    bool copy_page(page* dst, int timeout_ms=0);


    /**
     * @brief Ensures that the FIFO is ready for writing.
     *
     * When this function returns at least one tuple may be written
     * without blocking.
     */
    void ensure_write_ready() {
        if(_write_page->full())
            _flush_write_page(false);
    }
        

    /**
     * @brief Ensures that the FIFO is ready for reading.
     *
     * The call will block until a tuple becomes available for
     * reading, or EOF is encountered.
     *
     * @return false if EOF or timeout expired
     */
    bool ensure_read_ready(int timeout_ms=0) {
        // blocking attempt => only returns false if EOF
        return (_read_iterator->data != _read_end)
            || (_get_read_page(timeout_ms) == 1);
    }


    // non-blocking. Return 1 if ready, 0 if not ready, -1 if EOF
    int check_read_ready() {
	if(_read_iterator->data != _read_end)
	    return 1;
	return _get_read_page(-1);
    }

    
    bool terminate();

    
    bool send_eof();
    
    
    /**
     *  @brief Check if we have consumed all tuples from this
     *  buffer.
     *
     *  @return Returns true only if the producer has sent EOF and we
     *  have read every tuple of every page in this buffer.
     */
    bool eof() {
        return check_read_ready() < 0;
    }


    /**
     * @brief Should only be invoked by code responsible for merging
     * packets. The caller must not be holding the _lock mutex when
     * invoking this method.
     */
    void set_shared();


private:

    /**
     * @brief Should only be caller by the writer. The caller must be
     * holding the _lock mutex when invoking this method.
     */
    bool is_shared() {
        return _is_shared;
    }

    size_t _available_memory_writes() {
        return _memory_capacity - _pages_in_memory;
    }

    size_t _pages_in_fifo() {
        return _page_count;
    }

    size_t _available_fifo_reads() {
        return _available_reads;
    }

    void _termination_check() {
        if(is_terminated())
            THROW1(TerminatedBufferException, "Buffer closed unexpectedly");
    }

    void _set_read_page(page* p) {
        //TRACE(TRACE_ALWAYS, "Setting read page to %p\n", p);
	_read_page = p;
	_read_iterator = _read_page->begin();
	_read_end = _read_page->end()->data;
    }

    page* _alloc_page() {
        /* Allocate a new _write_page. */
        if (_free_pages.empty())
            /* Allocate using page::alloc. */
            return page::alloc(tuple_size());

        /* Allocate from free list. */
        page* p = _free_pages.front();
        _free_pages.pop_front();
        p->clear();
        return p;
    }

    void init();
    void destroy();

    /* These methods control access and waiting. */
    int  _get_read_page(int timeout);
    void _flush_write_page(bool done_writing);


    bool is_in_memory() {
        return
            _state.current() == tuple_fifo_state_t::IN_MEMORY ||
            _state.current() == tuple_fifo_state_t::IN_MEMORY_DONE_WRITING ||
            _state.current() == tuple_fifo_state_t::IN_MEMORY_TERMINATED;
    }

    bool is_done_writing() {
        return
            _state.current() == tuple_fifo_state_t::IN_MEMORY_DONE_WRITING ||
            _state.current() == tuple_fifo_state_t::ON_DISK_DONE_WRITING;
    }
    
    bool is_terminated() {
        return _state.current() == tuple_fifo_state_t::IN_MEMORY_TERMINATED
            || _state.current() == tuple_fifo_state_t::ON_DISK_TERMINATED;
    }


    void wait_for_reader();
    void ensure_reader_running();
    
    bool wait_for_writer(int timeout);
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
