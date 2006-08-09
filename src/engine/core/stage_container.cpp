/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/core/stage_container.h"
#include "engine/util/guard.h"
#include "trace.h"
#include "qpipe_panic.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <cstdio>
#include <cstring>



// include me last!!!
#include "engine/namespace.h"

#define TRACE_MERGING 0
#define TRACE_DEQUEUE 0



const unsigned int stage_container_t::NEXT_TUPLE_UNINITIALIZED = 0;
const unsigned int stage_container_t::NEXT_TUPLE_INITIAL_VALUE = 1;

// the "STOP" exception. Simply indicates that the stage should stop
// (not necessarily an "error"). Thrown by the adaptor's output(),
// caught by the container.
struct stop_exception { };


// container methods

/**
 *  @brief Stage constructor.
 *
 *  @param container_name The name of this stage container. Useful for
 *  printing debug information. The constructor will create a copy of
 *  this string, so the caller should deallocate it if necessary.
 */
stage_container_t::stage_container_t(const c_str &container_name,
				     stage_factory_t* stage_maker)
    : _container_name(container_name), _stage_maker(stage_maker)
{
  
    // mutex that serializes the creation of the worker threads
    pthread_mutex_init_wrapper(&_container_lock, NULL);
  
    // condition variable that worker threads can wait on to
    // deschedule until more packets arrive
    pthread_cond_init_wrapper(&_container_queue_nonempty, NULL);
}



/**
 *  @brief Stage container destructor. Should only be invoked after a
 *  shutdown() has successfully returned and no more worker threads
 *  are in the system. We will delete every entry of the container
 *  queue.
 */
stage_container_t::~stage_container_t(void) {
    // There should be no worker threads accessing the packet queue when
    // this function is called. Otherwise, we get race conditions and
    // invalid memory accesses.
  
    // destroy synch vars    
    pthread_mutex_destroy_wrapper(&_container_lock);
    pthread_cond_destroy_wrapper (&_container_queue_nonempty);
}



/**
 *  THE CALLER MUST BE HOLDING THE _container_lock MUTEX.
 */
void stage_container_t::container_queue_enqueue_no_merge(packet_list_t* packets) {
    _container_queue.push_back(packets);
    pthread_cond_signal_wrapper(&_container_queue_nonempty);
}



/**
 *  @brief Helper function used to add the specified packet as a new
 *  element in the _container_queue and signal a waiting worker
 *  thread.
 *
 *  THE CALLER MUST BE HOLDING THE _container_lock MUTEX.
 */
void stage_container_t::container_queue_enqueue_no_merge(packet_t* packet) {
    packet_list_t* packets = new packet_list_t();
    packets->push_back(packet);
    container_queue_enqueue_no_merge(packets);
}



/**
 *  @brief Helper function used to remove the next packet in this
 *  container queue. If no packets are available, wait for one to
 *  appear.
 *
 *  THE CALLER MUST BE HOLDING THE stage_lock MUTEX.
 */
packet_list_t* stage_container_t::container_queue_dequeue() {

    // wait for a packet to appear
    while ( _container_queue.empty() ) {
	pthread_cond_wait_wrapper( &_container_queue_nonempty, &_container_lock );
    }

    // remove available packet
    packet_list_t* plist = _container_queue.front();
    _container_queue.pop_front();
  
    return plist;
}



/**
 *  @brief Send the specified packet to this container. We will try to
 *  merge the packet into a running stage or into an already enqueued
 *  packet packet list. If we fail, we will wrap the packet in a
 *  packet_list_t and insert the list as a new entry in the container
 *  queue.
 *
 *  Merging will fail if the packet is already marked as non-mergeable
 *  (if its is_merge_enabled() method returns false). It will also
 *  fail if there are no "similar" packets (1) currently being
 *  processed by any stage AND (2) currently enqueued in the container
 *  queue.
 *
 *  @param packet The packet to send to this stage.
 */
void stage_container_t::enqueue(packet_t* packet) {


    assert(packet != NULL);


    // * * * BEGIN CRITICAL SECTION * * *
    critical_section_t cs(&_container_lock);


    // check for non-mergeable packets
    if ( !packet->is_merge_enabled() )  {
	// We are forcing the new packet to not merge with others.
	container_queue_enqueue_no_merge(packet);
        if (TRACE_MERGING)
            TRACE(TRACE_ALWAYS, "%s merging disabled\n",
                  packet->_packet_id.data());
	return;
	// * * * END CRITICAL SECTION * * *
    }


    // try merging with packets in merge_candidates before they
    // disappear or become non-mergeable
    list<stage_adaptor_t*>::iterator sit = _container_current_stages.begin();
    for( ; sit != _container_current_stages.end(); ++sit) {
	// try to merge with this stage
	stage_container_t::stage_adaptor_t* ad = *sit;
	if ( ad->try_merge(packet) ) {
	    /* packet was merged with this existing stage */
            if (TRACE_MERGING)
                TRACE(TRACE_ALWAYS, "%s merged into a stage next_tuple_on_merge = %d\n",
                      packet->_packet_id.data(),
                      packet->_next_tuple_on_merge);
	    return;
	    // * * * END CRITICAL SECTION * * *
	}
    }


    // If we are here, we could not merge with any of the running
    // stages. Don't give up. We can still try merging with a packet in
    // the container_queue.
    list<packet_list_t*>::iterator cit = _container_queue.begin();
    for ( ; cit != _container_queue.end(); ++cit) {
	
	packet_list_t* cq_plist = *cit;
	packet_t* cq_packet = cq_plist->front();


	// No need to acquire queue_pack's merge mutex. No other
	// thread can touch it until we release _container_lock.
    
	// We need to check queue_pack's mergeability flag since some
	// packets are non-mergeable from the beginning. Don't need to
	// grab its merge_mutex because its mergeability status could
	// not have changed while it was in the queue.
	if ( cq_packet->is_mergeable(packet) ) {
	    // add this packet to the list of already merged packets
	    // in the container queue
	    cq_plist->push_back(packet);
            if (TRACE_MERGING)
                TRACE(TRACE_ALWAYS, "%s merged into existing packet list\n",
                      packet->_packet_id.data());
	    return;
	    // * * * END CRITICAL SECTION * * *
	}
    }
    

    if (TRACE_MERGING)
        TRACE(TRACE_ALWAYS, "%s could not be merged\n",
              packet->_packet_id.data());
    
    
    // No work sharing detected. We can now give up and insert the new
    // packet into the stage_queue.
    container_queue_enqueue_no_merge(packet);
    // * * * END CRITICAL SECTION * * *
};



/**
 *  @brief Worker threads for this stage should invoke this
 *  function. It will return when the stage shuts down.
 */
void stage_container_t::run() {

    while (1) {
	

	// Wait for a packet to become available. We release the
	// _container_lock in container_queue_dequeue() if we end up
	// actually waiting.
	critical_section_t cs(&_container_lock);
	packet_list_t* packets = container_queue_dequeue();
	

	// error checking
	assert( packets != NULL );
	assert( !packets->empty() );
        if (TRACE_DEQUEUE) {
            packet_t* head_packet = *(packets->begin());
            TRACE(TRACE_ALWAYS, "Processing %s\n",
                  head_packet->_packet_id.data());
        }


        // Construct an adaptor to work with. If this is expensive, we
        // can construct the adaptor before the dequeue and invoke
        // some init() function to initialize the adaptor with the
        // packet list.
	stage_adaptor_t adaptor(this, packets, packets->front()->_output_filter->input_tuple_size());

        
	// Add new stage to the container's list of active stages. It
	// is better to release the container lock and reacquire it
	// here since stage construction can take a long time.
        _container_current_stages.push_back( &adaptor );
	cs.exit();

        
        // create stage
        guard<stage_t> stage = _stage_maker->create_stage();
        adaptor.run_stage(stage);

	
        // remove active stage
	critical_section_t cs_remove_active_stage(&_container_lock);
        _container_current_stages.remove(&adaptor);
	cs_remove_active_stage.exit();
        
        
	// TODO: check for container shutdown
    }
}




// stage adaptor methods


stage_container_t::stage_adaptor_t::stage_adaptor_t(stage_container_t* container,
                                                    packet_list_t* packet_list,
                                                    size_t tuple_size)
    : adaptor_t(page::alloc(tuple_size)),
      _container(container),
      _packet_list(packet_list),
      _next_tuple(NEXT_TUPLE_INITIAL_VALUE),
      _still_accepting_packets(true),
      _cancelled(false)
{
    
    assert( !packet_list->empty() );
    
    pthread_mutex_init_wrapper(&_stage_adaptor_lock, NULL);
    
    packet_list_t::iterator it;
    
    // We only need one packet to provide us with inputs. We
    // will make the first packet in the list a "primary"
    // packet. We can destroy the packet subtrees in the other
    // packets in the list since they will NEVER be used.
    it = packet_list->begin();
    _packet = *it;

    // Record next_tuple field in ALL packets, even the
    // primary.
    for (it = packet_list->begin(); it != packet_list->end(); ++it) {
        packet_t* packet = *it;
        packet->_next_tuple_on_merge = NEXT_TUPLE_INITIAL_VALUE;
    }
}



/**
 *  @brief Try to merge the specified packet into this stage. This
 *  function assumes that packet has its mergeable flag set to
 *  true.
 *
 *  This method relies on the stage's current state (whether
 *  _stage_accepting_packets is true) as well as the
 *  is_mergeable() method used to compare two packets.
 *
 *  @param packet The packet to try and merge.
 *
 *  @return true if the merge was successful. false otherwise.
 */
bool stage_container_t::stage_adaptor_t::try_merge(packet_t* packet) {
 
    // * * * BEGIN CRITICAL SECTION * * *
    critical_section_t cs(&_stage_adaptor_lock);


    if ( !_still_accepting_packets ) {
	// stage not longer in a state where it can accept new packets
	return false;
	// * * * END CRITICAL SECTION * * *	
    }

    // The _still_accepting_packets flag cannot change since we are
    // holding the _stage_adaptor_lock. We can also safely access
    // _packet for the same reason.
 

    // Mergeability is an equality relation. We can check whether
    // packet is similar to the packets in this stage by simply
    // comparing it with the stage's primary packet.
    if ( !_packet->is_mergeable(packet) ) {
	// packet cannot share work with this stage
	return false;
	// * * * END CRITICAL SECTION * * *
    }
    
    
    // If we are here, we detected work sharing!
    _packet_list->push_front(packet);
    packet->_next_tuple_on_merge = _next_tuple;

    // * * * END CRITICAL SECTION * * *
    return true;
}



/**
 *  @brief Outputs a page of tuples to this stage's packet set. The
 *  caller retains ownership of the page.
 */
void stage_container_t::stage_adaptor_t::output_page(page* p) {

    packet_list_t::iterator it, end;
    unsigned int next_tuple;
    
    
    critical_section_t cs(&_stage_adaptor_lock);
    it  = _packet_list->begin();
    end = _packet_list->end();
    _next_tuple += p->tuple_count();
    next_tuple = _next_tuple;
    cs.exit();

    
    // Any new packets which merge after this point will not 
    // receive this page.
    

    bool packets_remaining = false;
    while (it != end) {


        packet_t* curr_packet = *it;
        bool terminate_curr_packet = false;
        try {
            // Drain all tuples in output page into the current packet's
            // output buffer.
            page::iterator page_it = p->begin();
            while(page_it != p->end()) {

                // apply current packet's filter to this tuple
                tuple_t in_tup = page_it.advance();
                if(curr_packet->_output_filter->select(in_tup)) {

                    // this tuple selected by filter!

                    // allocate space in the output buffer and project into it
                    tuple_t out_tup = curr_packet->output_buffer()->allocate();
                    curr_packet->_output_filter->project(out_tup, in_tup);
                }
            }
            
            // If this packet has run more than once, it may have received
            // all the tuples it needs. Check for this case.
            if ( next_tuple == curr_packet->_next_tuple_needed )
                // This packet has received all tuples it needs! Another
                // reason to terminate this packet!
                terminate_curr_packet = true;
        

        } catch(TerminatedBufferException &e) {
            // The consumer of the current packet's output
            // buffer has terminated the buffer! No need to
            // continue iterating over output page.
            terminate_curr_packet = true;
        }
        
        
        // check for packet termination
        if (terminate_curr_packet) {
            // Finishing up a stage packet is tricky. We must treat
            // terminating the primary packet as a special case. The
            // good news is that the finish_packet() method handle all
            // special cases for us.
            finish_packet(curr_packet);
            it = _packet_list->erase(it);
            continue;
        }
 

        ++it;
        packets_remaining = true;
    }
    
    
    // no packets that need tuples?
    if ( !packets_remaining )
        throw stop_exception();
}



/**
 *  @brief Send EOF to the packet's output buffer. Delete the buffer
 *  if the consumer has already terminated it. If packet is not the
 *  primary packet for this stage, destroy its subpackets and delete
 *  it.
 *
 *  @param packet The packet to terminate.
 */
void stage_container_t::stage_adaptor_t::finish_packet(packet_t* packet) {

    
    // packet output buffer
    guard<tuple_fifo> output_buffer = packet->release_output_buffer();
    if ( output_buffer->send_eof() )
        // Consumer has not already terminated this buffer! It is now
        // responsible for deleting it.
        output_buffer.release();

    // packet input buffer(s)
    if ( packet != _packet ) {
        // since we are not the primary, can happily destroy packet
        // subtrees
        delete packet;
    }
}



/**
 *  @brief When a worker thread dequeues a new packet list from the
 *  container queue, it should create a stage_adaptor_t around that
 *  list, create a stage to work with, and invoke this method with the
 *  stage. This function invokes the stage's process() method with
 *  itself and "cleans up" the stage's packet list when process()
 *  returns.
 *
 *  @param stage The stage providing us with a process().
 */
void stage_container_t::stage_adaptor_t::run_stage(stage_t* stage) {


    // error checking
    assert( stage != NULL );

    
    // run stage-specific processing function
    bool error = false;
    try {
        stage->init(this);
        stage->process();
        flush();
    } catch(stop_exception &) {
        // no error
        TRACE(TRACE_DEBUG, "process() ended early\n");
    } catch(...) {
        // error!
        TRACE(TRACE_DEBUG, "process() encountered an error\n");
        error = true;
    }

    // if we are still accepting packets, stop now
    stop_accepting_packets();
    if(error)
        abort_queries();
    else
        cleanup();

}



/**
 *  @brief Cleanup after successful processing of a stage.
 *
 *  Walk through packet list. Invoke finish_packet() on packets that
 *  merged when _next_tuple == NEXT_TUPLE_INITIAL_VALUE. Erase these
 *  packets from the packet list.
 *
 *  Take the remain packet list (of "unfinished" packets) and
 *  re-enqueue it.
 *
 *  Invoke terminate_inputs() on the primary packet and delete it.
 */
void stage_container_t::stage_adaptor_t::cleanup() {

    // walk through our packet list
    packet_list_t::iterator it;
    for (it = _packet_list->begin(); it != _packet_list->end(); ) {
        
	packet_t* curr_packet = *it;

        // Check for all packets that have been with this stage from
        // the beginning. If we haven't invoked finish_packet() on the
        // primary yet, we're going to do it now.
        if ( curr_packet->_next_tuple_on_merge == NEXT_TUPLE_INITIAL_VALUE ) {

            // The packet is finished. If it is not the primary
            // packet, finish_packet() will delete it.
            finish_packet(curr_packet);

            it = _packet_list->erase(it);
            continue;
        }
        
        // The packet is not finished. Simply update its progress
        // counter(s). The worker thread that picks up this packet
        // list should set the _stage_next_tuple_on_merge fields to
        // NEXT_TUPLE_INITIAL_VALUE.
        curr_packet->_next_tuple_needed =
            curr_packet->_next_tuple_on_merge;
        curr_packet->_next_tuple_on_merge = NEXT_TUPLE_UNINITIALIZED;
        ++it;
    }


    // Re-enqueue incomplete packets if we have them
    if ( _packet_list->empty() ) {
	delete _packet_list;
    }
    else {
        // Hand off ownership of the packet list back to the
        // container.
	_container->container_queue_enqueue_no_merge(_packet_list);
    }
    _packet_list = NULL;

    
    // Terminate inputs of primary packet. finish_packet() already
    // took care of its output buffer.
    assert(_packet != NULL);
    delete _packet;
    _packet = NULL;
}



/**
 *  @brief Cleanup after unsuccessful processing of a stage.
 *
 *  Walk through packet list. For each non-primary packet, invoke
 *  terminate() on its output buffer, invoke destroy_subpackets() to
 *  destroy its inputs, and delete the packet.
 *
 *  Invoke terminate() on the primary packet's output buffer, invoke
 *  terminate_inputs() to terminate its input buffers, and delete it.
 */
void stage_container_t::stage_adaptor_t::abort_queries() {


    // handle non-primary packets in packet list
    packet_list_t::iterator it;
    for (it = _packet_list->begin(); it != _packet_list->end(); ++it) {
        

	packet_t* curr_packet = *it;


        if ( curr_packet != _packet ) {
            // packet is non-primary
            delete curr_packet;
        }
    }

    
    // handle primary packet
    delete _packet;
}



#include "engine/namespace.h"
