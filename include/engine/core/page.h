/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _PAGE_H
#define _PAGE_H

#include "engine/thread.h"
#include "engine/util/guard.h"

#include <new>



// include me last!!!
#include "engine/namespace.h"

extern void set_default_page_size(size_t page_size);
extern size_t get_default_page_size();


/**
 *  @brief Wapper class for a page header that stores the page's
 *  size. The constructor is private to prevent stray headers from
 *  being created in the code. We instead export a static alloc()
 *  function that allocates a new page and places a header at the
 *  base.
 */
class page_t {

 private:

    size_t _page_size;
    
 public:

    page_t *next;

 public:

    /** overrides page memory management */
    void* operator new(size_t n, size_t page_size=get_default_page_size());
    void operator delete(void* p);

    // leave initialization to the allocator
    page_t() { }
    
    size_t page_size() const {
        return _page_size;
    }

private:
    // Prevent attempts to copy pages. The class cannot be allocated
    // on the stack because it needs more space than sizeof(page_t).
    page_t(const page_t &other);
    page_t &operator=(const page_t &other);
};



/**
 *  @brief Thread-safe page buffer. This class allows one thread to
 *  safely pass pages to another. The producing thread gives up
 *  ownership of the pages it sends to the buffer.
 */
class page_buffer_t {

private:

    // number of pages allowed in the buffer
    int capacity;

    // tuning parameter
    int threshold;

    // synch vars
    pthread_mutex_t lock;
    pthread_cond_t read_notify;
    pthread_cond_t write_notify;

    // The current committed size of the buffer. The reader and writer
    // communicate through this variable, but it isn't always up to
    // date. In order to avoid serializing unnecessarily, the reader
    // and writer do not update this regularly. The true size of the
    // buffer at any given moment is 'size + uncommitted_write_count -
    // uncommitted_read_count'.
    volatile int size;

    // flags used to close the buffer
    volatile bool terminated;
    volatile bool done_writing;

    // the singly-linked page list this buffer manages
    page_t *head;
    page_t *tail;

    // Conservative size estimates maintained by the reader and
    // writer, respectively. They are not necessarily accurate, but
    // will never cause over/underflow.
    int read_size_guess;
    int write_size_guess;

    // the number of reads/writes performed since last updating 'size'.
    int uncommitted_read_count;
    int uncommitted_write_count;
    
public:

    page_buffer_t(int capacity, int threshold=-1) {
        init(capacity, threshold);
    }
    
    
    ~page_buffer_t();
    
    
    /**
     *  @brief Check if the buffer is empty (if the producer has
     *  stopped writing) and the consumer has completely drained the
     *  buffer contents.
     *
     *  This method will not check for a closed buffer. This is
     *  intentional. We must be able to detect and handle close() on
     *  actual reads and writes. Otherwise, we introduce races.
     */
    bool empty() {
        return stopped_writing() && read_size_guess == 0;
    }
    
    
    int  read(page_t*& page);
    int  write(page_t* page);
    bool check_readable();
    bool check_writable();
    
    bool terminate();
    bool stop_writing();

    bool is_terminated() { return terminated; }
    bool stopped_writing() { return done_writing; }
    
private:

    void init(int capacity, int threshold);


    /**
     *  @brief Called by the consumer. Checks whether it is time for
     *  the consumer to acquire the buffer lock and update its
     *  estimate of the number of pages in the buffer.
     *
     *  See comments in the read() implementation about the guess == 1
     *  corner case.
     *
     *  @return true if the consumer needs to update its
     *  estimate. false otherwise.
     */
    bool must_verify_read() { return read_size_guess == 1; }


    /**
     *  @brief Called by the producer. Checks whether it is time for
     *  the producer to acquire the buffer lock and update its
     *  estimate of the amount of free space in the buffer.
     *
     *  @return true if the producer needs to update its
     *  estimate. false otherwise.
     */
    bool must_verify_write() { return write_size_guess == capacity; }


    /**
     *  @brief Called by the consumer. Commits consumer's uncommitted
     *  reads so the producer can see how much space is actually in
     *  the buffer. THE CALLER MUST HOLD THE BUFFER LOCK!
     */
    void commit_reads() {
        size -= uncommitted_read_count;
        uncommitted_read_count = 0;
    }


    /**
     *  @brief Called by the producer. Commits producer's uncommitted
     *  writes so the consumer can see how much data is actually in
     *  the buffer. THE CALLER MUST HOLD THE BUFFER LOCK!
     */
    void commit_writes() {
        size += uncommitted_write_count;
        uncommitted_write_count = 0;
    }
};



/**
 *  @brief Ensures that a list of pages is freed.
 */
struct page_list_guard_t : public pointer_guard_base_t<page_t, page_list_guard_t> {

    page_list_guard_t(page_t *ptr=NULL)
        : pointer_guard_base_t<page_t, page_list_guard_t>(ptr)
    {
    }

    struct Action {
        void operator()(page_t *ptr) {
            while(ptr) {
                page_t *page = ptr;
                ptr = page->next;
                delete page;
            }
        }
    };
    
    void append(page_t *ptr) {
        assert(ptr);
        ptr->next = release();
        assign(ptr);
    }

    page_list_guard_t &operator=(const page_list_guard_t::temp_ref_t &ref) {
        assign(ref._ptr);
        return *this;
    }

private:

    page_list_guard_t &operator=(page_list_guard_t &);

};



#include "engine/namespace.h"



#endif
