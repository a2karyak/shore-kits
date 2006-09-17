/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/thread.h"
#include "engine/core/page.h"
#include "engine/core/exception.h"
#include <cstring>


// include me last!
#include "engine/namespace.h"




/**
 *  @brief Allocates a new page of size (page_size), ignoring the
 *  requested size.
 *
 *  This implementation uses a thread-specific trash stack to avoid
 *  calling the global memory allocator (and hopefully improve
 *  locality as well). Non-standard page sizes are not eligible for
 *  linking, however -- too complicated to be worth it.
 */
void* page_t::operator new(size_t, size_t page_size) {
    union {
        void* ptr;
        page_t *page;
    } pun;
    pun.ptr = ::operator new(page_size);

    // initialize the page header
    pun.page->_page_size = page_size;
    pun.page->next = NULL;
    return pun.ptr;
}



/**
 *  @brief Frees a page
 *
 *  This implementation returns standard-sized pages to the trash
 *  stack, freeing non-standard pages immediately.
 */
void page_t::operator delete(void* ptr) {
    ::operator delete(ptr);
}



/**
 *  @brief Initialize a page buffer (can be invoked by subclasses).
 *
 *  @param capacity The maximum number of pages to store in this
 *  buffer.
 *
 *  @param threshold If the buffer fills up, the producer will wait
 *  for it to contain at least this many free slots. If the buffer
 *  becomes empty, the consumer will wait for it to contain at least
 *  this many pages.
 */
void page_buffer_t::init(int capacity, int threshold) {

    page_buffer_t::capacity = capacity;
    if(threshold < 0)
        page_buffer_t::threshold = (int) (.3*capacity);
    else
        page_buffer_t::threshold = threshold;
    
    size = 0;

    // Pretend that we have a page already in the buffer. The consumer
    // can't remove this imaginary page until the buffer is closed.
    read_size_guess  = 1;
    write_size_guess = 0;

    head = tail = NULL;
    terminated = done_writing = false;

    pthread_mutex_init_wrapper(&lock, NULL);
    pthread_cond_init_wrapper(&read_notify, NULL);
    pthread_cond_init_wrapper(&write_notify, NULL);

    uncommitted_write_count = uncommitted_read_count = 0;
}



/**
 *  @brief This function can be called by either the producer or the
 *  consumer. Check if either the buffer has been terminated or if the
 *  producer has stopped writing to the buffer. If the buffer is not
 *  terminated and the producer has not stopped writing, we atomically
 *  mark the buffer as terminated, wake any thread that is waiting,
 *  and return true.
 *
 *  @return true if the buffer is successfully terminated, in which
 *  case the caller gives up buffer ownership; he should not perform
 *  any further operations on it. false if either the buffer is
 *  already closed or if the producer has invoked stop_writing(). In
 *  this case, the caller now has total ownership of this buffer; he
 *  becomes responsible for deleting it.
 */

bool page_buffer_t::terminate() {


    // * * * BEGIN CRITICAL SECTION * * *
    critical_section_t cs(&lock);

    
    if (done_writing || terminated) {
        return false;
        // * * * END CRITICAL SECTION * * *
    }

    // Don't bother updating the buffer size. The producer will be
    // prevented from adding to the buffer by the fact that we have
    // marked the buffer closed (with the done_reading flag).
    
    // mark the buffer closed
    terminated = true;

    // wake the producer
    pthread_cond_signal_wrapper(&write_notify);
    
    // wake the consumer (since producer may call this function)
    pthread_cond_signal_wrapper(&read_notify);
    
    return true;
    // * * * END CRITICAL SECTION * * *
}



/**
 *  @brief This function can only be called by the producer. Check if
 *  the buffer has been terminated. If the buffer is not terminated,
 *  we atomically mark the buffer as closed by the producer, wake the
 *  consumer if it is waiting, and return true.
 *
 *  @return true if the producer has given up buffer ownership to the
 *  consumer; in this case, the producer should not perform any
 *  further operations on this buffer. false if the buffer has been
 *  terminated, in which case the producer gains total ownership of
 *  the buffer; he is responsible for deleting it.
 */

bool page_buffer_t::stop_writing() {

    // * * * BEGIN CRITICAL SECTION * * *
    critical_section_t cs(&lock);
    
    if (terminated) {
        return false;
        // * * * END CRITICAL SECTION * * *
    }
        
    // Do a final update of the buffer size so the consumer does not
    // miss any of our recent additions.
    commit_writes();

    // Mark the buffer so the consumer knows to stop once it has done
    // a final drain.
    done_writing = true;

    // notify the consumer
    pthread_cond_signal_wrapper(&read_notify);
        
    // * * * END CRITICAL SECTION * * *
    return true;
}



/**
 *  @brief Non-blocking check for whether pages are available for
 *  reading. If there are no pages available (if this function returns
 *  false), it is the responsibility of the caller to check (using
 *  is_terminated() and stopped_writing()) whether the buffer has been
 *  terminated, whether the producer has invoked stop_writing(), or
 *  whether the buffer is simply empty.
 *
 *  @brief true if there are pages available for reading. false
 *  otherwise.
 */
bool page_buffer_t::check_readable() {

    // No need to synchronize here. There is always a window between a
    // time we determine that terminate is false and the next buffer
    // operation where a terminate() could have been invoked.
    if (terminated)
        return false;

    // If our guess says buffer is not readable, update guess and
    // check again.
    if(must_verify_read()) {
        critical_section_t cs(&lock);

        // The buffer may be closed here, but we can still continue
        // normally. We will detect the close when we actually try to
        // read.
        commit_reads();

        // Invariant: read_size_guess must be positive until the
        // producer has closed his end of the buffer and there are no
        // more finished pages available for reading. size however,
        // starts as 0 since there is nothing in the buffer. This
        // check keeps us from assigning this 0 size to
        // read_size_guess prematurely.
        if(size == 0 && stopped_writing())
            read_size_guess = size;
    }
    
    // pages available?
    if(read_size_guess)
        return true;

    // regardless of eof or slow producer, we return false
    return false;
}



/**
 *  @brief Non-blocking check for whether the writer can put data into
 *  the buffer. If the writer cannot (if this function returns false),
 *  it is the responsibility of the caller to check (using the
 *  closed() method) whether the buffer has been closed or whether the
 *  size has reached the buffer's maximum capacity.
 *
 *  @brief true if the writer can put data into the buffer. false
 *  otherwise.
 */
bool page_buffer_t::check_writable() {

    // No need to synchronize here. There is always a window between a
    // time we determine that terminate is false and the next buffer
    // operation where a terminate() could have been invoked.
    if (terminated)
        return false;
    
    // If our guess says buffer is not writable, update guess and
    // check again.
    if(must_verify_write()) {
        critical_section_t cs(&lock);

        // The buffer may be closed here, but we can still continue
        // normally. We will detect the close when we actually try to
        // write.

        commit_writes();
        write_size_guess = size;
    }

    // ready, or not?
    return write_size_guess < capacity;
}



/**
 *  @brief This function can only be called by the consumer. Read a
 *  page from the buffer. If we are down to one page in the buffer,
 *  wait for either the producer to finish or for at least threshold
 *  pages to appear in the buffer.
 *
 *  @return a page read from the buffer. NULL if the buffer has been
 *  closed and no more pages are available.
 *
 *  @throw TerminatedBufferException if buffer has been terminated.
 */

page_t* page_buffer_t::read() {

    // Treat the one-page-left case separately. We avoid the
    // head==tail corner case by never allowing the buffer to empty
    // completely until the producer is finished)
    if(must_verify_read()) {

        // * * * BEGIN CRITICAL SECTION * * *
        critical_section_t cs(&lock);

        // check for close
        if (terminated)
            throw EXCEPTION(TerminatedBufferException, "");

        // We are probably going to wait for the buffer to fill
        // up. Before we deschedule ourselves, update the buffer size.
        commit_reads();
            
        // We want to avoid a thrashing scenario where the producer
        // inserts one page and immediately yields to us, we remove
        // the page and immediately yield to him... As long as the
        // producer is still writing, we might as well wait for it to
        // produce enough to meet our threshold.
        while(size < threshold && !done_writing) {

	    // Tell the producer to write more... why? Why would the
	    // producer be waiting if there is still plenty of space
	    // in the buffer?
            pthread_cond_signal_wrapper(&write_notify);

	    // wait for the producer to write more
            pthread_cond_wait_wrapper(&read_notify, &lock);
            
            // since we gave up the buffer lock when we waited, the
            // buffer could have been closed by now
            if (terminated)
                throw EXCEPTION(TerminatedBufferException, "");
	}


        // We know there are at least size things in the buffer (there
        // are actually size + uncommitted_write_count)
        read_size_guess = size;
        
        // * * * END CRITICAL SECTION * * *
    }


    // empty (and therefore also done writing)?
    if(read_size_guess == 0) 
        return NULL;
    
    
    // proceed -- known not empty
    page_t* page = (page_t *)head;
    head = head->next;


    // update our counts
    read_size_guess--;
    uncommitted_read_count++;

    
    // occasionally signal the producer to let both threads work in
    // parallel
    if(uncommitted_read_count == threshold) {

        // * * * BEGIN CRITICAL SECTION * * *
        critical_section_t cs(&lock);

        // The buffer may be closed here, but we have already removed
        // a page, so we should return success. We will catch the
        // close on the next write() operation.
        
        // update the size and notify the producer
        commit_reads();
        pthread_cond_signal_wrapper(&write_notify);
        
        // * * * END CRITICAL SECTION * * *
    }
    
    return page;
}



/**
 *  @brief Write a page to the buffer. If the buffer is full (if there
 *  are capacity pages), wait until the free space (capacity - size)
 *  falls below threshold. If the consumer has closed his end of the
 *  buffer, return immediately without inserting.
 *
 *  @param page The page to write. We actually insert the specified
 *  page into the buffer, so the caller must give up its ownership of
 *  it.
 *
 *  @return true if the page has been written to the buffer. false if
 *  the buffer has been terminated. If the buffer has been terminated,
 *  the page will not be inserted.
 */

bool page_buffer_t::write(page_t* page) {
    
    assert( page != NULL );
    
    
    // possibly full? (no corner case to worry about here)
    if(must_verify_write()) {

        // * * * BEGIN CRITICAL SECTION * * *
        critical_section_t cs(&lock);

        // check for close
        if (terminated)
            return false;

        // update the size and notify the consumer
        commit_writes();
                
        // Want to avoid the same thrashing scenario with read(). This
        // time, wait for amount of free space in buffer to drop below
        // threshold.	
        while((size > (capacity - threshold)) && !terminated) {

	    // Tell the consumer to read more data from the
	    // buffer... why? Why would the consumer be waiting if
	    // there is still plenty of data in the buffer?
            pthread_cond_signal_wrapper(&read_notify);

	    // wait for the consumer to read more
            pthread_cond_wait_wrapper(&write_notify, &lock);

        }
        
	// update the guess and the size
        write_size_guess = size;
        
        // * * * END CRITICAL SECTION * * *
    }

    // since we gave up the buffer lock when we waited, the
    // buffer could have been closed by now
    if (terminated)
        return false;

    // proceed -- known not full. Corner case: very first write is to
    // an empty buffer, meaning the head and tail pointers are both
    // NULL. It's unsafe to test for this with the value of 'head'.
    if(write_size_guess == 0)
        head = page;
    else
        tail->next = page;
    tail = page;


    // update counts
    write_size_guess++;
    uncommitted_write_count++;


    // occasionally signal the consumer to encourage both threads to
    // work in parallel
    if(uncommitted_write_count == threshold) {
        // * * * BEGIN CRITICAL SECTION * * *
        critical_section_t cs(&lock);

        // The buffer may be closed here, but we have already inserted
        // the page, so we should return success. We will catch the
        // close on the next write() operation.

        // update the count and notify the consumer
        commit_writes();
        pthread_cond_signal_wrapper(&read_notify);
            
        // * * * END CRITICAL SECTION * * *
    }
    

    return true;
}



/**
 *  @brief Page buffer destructor. Current invokes free() on all
 *  pages.
 */

page_buffer_t::~page_buffer_t() {
    // free all remaining (unclaimed) pages
    while(head) {
        page_t* page = head;
        head = head->next;
        delete page;
    }
}



#include "engine/namespace.h"
