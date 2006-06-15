/* -*- mode:C++; c-basic-offset:4 -*- */

#include "thread.h"
#include "tuple.h"



// include me last!
#include "engine/namespace.h"



/**
 *  @brief This function can only be called by the producer. If the
 *  current write page is full, try to write it to the internal
 *  page_buffer_t and allocate a new page. If this buffer is full (if
 *  it already has the maximum number of pages allocated), this
 *  function will block until more space becomes available or the
 *  buffer is closed.
 *
 *  @return 0 if the buffer can be written to. Non-zero if the buffer
 *  has been terminated.
 */

int tuple_buffer_t::check_page_full() {
    
    // write page will be NULL if we have already sent EOF
    if ( write_page == NULL ) {
        TRACE(TRACE_ALWAYS, "NULL write_page. Already sent EOF?\n");
        QPIPE_PANIC();
    }

    if(write_page->full()) {

        // page is full
        if(flush_write_page()) {
            // buffer has been terminated
            return -1;
        }
        
        // TODO: something besides malloc
        // TODO: deal with memory errors
        write_page = tuple_page_t::alloc(tuple_size, malloc, _page_size);
        assert(write_page != NULL);

        // we now have space in the buffer
        return 0;
    }

    // otherwise just test for cancellation
    return page_buffer.is_terminated();
}



/**
 *  @brief This function can only be called by the consumer. Block
 *  until either inputs become available or the producer closes the
 *  buffer (sending EOF).
 *
 *  @return 0 if input if available. 1 if the producer has sent EOF to
 *  this buffer and no more pages are available. -1 if the buffer has
 *  been terminated.
 */

int tuple_buffer_t::wait_for_input() {
 
    if((read_page != NULL) && (read_iterator != read_page->end()))
	return 0;
   
    // wait for the next page to arrive
    page_t* page;
    int read_ret = page_buffer.read(page);
    if (read_ret)
        return read_ret;
        
    read_page = tuple_page_t::mount(page);
    read_iterator = read_page->begin();
    return 0;
}



/**
 *  @brief This function can only be called by the consumer. Retrieve
 *  a tuple from the buffer. This function blocks the calling thread
 *  if the buffer is empty and still open (if there has been no EOF or
 *  termination).
 *
 *  @param tuple On success, this tuple is set to point to the tuple
 *  most recently removed from this page.
 *
 *  @return 0 if a tuple is removed from the buffer. 1 if the buffer
 *  is empty and the producer has sent EOF. -1 if the buffer has been
 *  terminated.
 */

int tuple_buffer_t::get_tuple(tuple_t &rec) {

    // make sure there is a valid page
    int wait_ret = wait_for_input();
    if (wait_ret)
        return wait_ret;
    
    rec = *read_iterator++;
    return 0;
}



/**
 *  @brief This function can only be called by the producer. It
 *  signals the consumer that no more tuples will be inserted into the
 *  buffer. Once the tuples currently in the buffer are read, future
 *  read operations will fail. If there is a consumer waiting for
 *  tuples, we will wake it up.
 *
 *  @brief true if EOF was successfully sent, in which case, the
 *  producer should perform no other operations on this buffer. false
 *  if the tuple buffer has been terminated, in which case the
 *  producer becomes its sole owner; he is responsible for deleting
 *  it.
 */

bool tuple_buffer_t::send_eof() {

    // write page will be NULL if we have already sent EOF
    if ( write_page == NULL ) {
        TRACE(TRACE_ALWAYS, "NULL write_page. Sending EOF more than once?\n");
        QPIPE_PANIC();
    }

    if (!write_page->empty()) {
        if ( flush_write_page() )
            // buffer has been terminated!
            return false;
    }

    return page_buffer.stop_writing();
}



/**
 *  @brief This function can be called by either the producer or the
 *  consumer. Future insertions and removals from the buffer will
 *  fail.
 *
 *  @return true if the buffer is successfully terminated. In this
 *  case, the caller should perform no additional operations with this
 *  buffer. false if called by the consumer and the producer has
 *  already invoked send_eof() or if the "other side" (can be producer
 *  or consumer) has invoked terminate(). If this function returns
 *  false, the caller becomes the sole owner of this tuple buffer; he
 *  is responsible for deleting it.
 */

bool tuple_buffer_t::terminate() {
    return page_buffer.terminate();
}



/**
 *  @brief Initialize this page buffer with a single empty tuple
 *  page.
 *
 *  @param _tuple_size The size of the tuples this buffer will store.
 *
 *  @param page_size The size of the pages to use for this buffer.
 *
 *  @return 0 on successful initialization. Negative value on error
 *  (if the allocate fails to create a page).
 */

int tuple_buffer_t::init(size_t _tuple_size, size_t page_size) {

    write_page = tuple_page_t::alloc(_tuple_size, malloc, page_size);
    if (write_page == NULL)
	return -1;
	
    input_arrived = false;
    tuple_size = _tuple_size;
    _page_size = page_size;
    read_page = NULL;

    return 0;
}



#include "engine/namespace.h"
