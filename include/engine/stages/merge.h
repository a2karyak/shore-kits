/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __MERGE_H
#define __MERGE_H

#include "stage.h"
#include "dispatcher.h"

#include <vector>

using std::vector;



using namespace qpipe;



/**
 *  @brief Packet definition for an N-way merge stage
 */
struct merge_packet_t : public packet_t {

    static const char* PACKET_TYPE;
    typedef vector<tuple_buffer_t*> buffer_list_t;
    
    buffer_list_t       _input_buffers;
    tuple_comparator_t* _comparator;


    /**
     *  @brief aggregate_packet_t constructor.
     *
     *  @param packet_id The ID of this packet. This should point to a
     *  block of bytes allocated with malloc(). This packet will take
     *  ownership of this block and invoke free() when it is
     *  destroyed.
     *
     *  @param output_buffer The buffer where this packet should send
     *  its data. A packet DOES NOT own its output buffer (we will not
     *  invoke delete or free() on this field in our packet
     *  destructor).
     *
     *  @param output_filter The filter that will be applied to any
     *  tuple sent to output_buffer. The packet OWNS this filter. It
     *  will be deleted in the packet destructor.
     *
     *  @param output_filter The filter that will be applied to any
     *  tuple sent to output_buffer. The packet OWNS this filter. It
     *  will be deleted in the packet destructor.
     *
     *  @param input_buffers A list of tuple_buffer_t pointers. This
     *  is the set of inputs that we are merging. This list should be
     *  set up by the meta-stage that creates this merge_packet_t. We
     *  will take ownership of the tuple_buffer_t's, but not the list
     *  itself. We will copy the list.
     *
     *  @param comparator A comparator for the tuples in our input
     *  buffers. This packet owns this comparator.
     */
    merge_packet_t(char*                packet_id,
                   tuple_buffer_t*      output_buffer,
                   tuple_filter_t*      output_filter,
                   const buffer_list_t& input_buffers,
                   tuple_comparator_t*  comparator)
	: packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter, false),
          _input_buffers(input_buffers),
          _comparator(comparator)
    {
    }


    virtual void destroy_subpackets() {
        // MERGE has no subpackets. This should never be invoked
        // anyway since an FDUMP is inherently non-mergeable.
        TRACE(TRACE_ALWAYS, "MERGE is non-mergeable!\n");
        QPIPE_PANIC();
    }    
    
    
    virtual void terminate_inputs() {	
        
        buffer_list_t::iterator it;
        for (it= _input_buffers.begin(); it != _input_buffers.end(); ) {

            tuple_buffer_t* _input_buffer = *it;

            // terminate current input buffer
            if ( !_input_buffer->terminate() ) {
                // Producer has already terminated this buffer! We are now
                // responsible for deleting it.
                delete _input_buffer;
            }

            it = _input_buffers.erase(it);
        }
    }
};



/**
 *  @brief Merge stage that merges N sorted inputs into one sorted
 *  output run.
 */
class merge_stage_t : public stage_t {

private:

    struct buffer_head_t {
        // for the linked list
        buffer_head_t *next;
        
        tuple_buffer_t *buffer;
        tuple_comparator_t *cmp;
        array_guard_t<char> data;
        tuple_t tuple;
        key_tuple_pair_t item;
        buffer_head_t() { }
        bool init(tuple_buffer_t *buf, tuple_comparator_t *c);
        int has_tuple();
    };
    
    buffer_head_t *_head_list;
    tuple_comparator_t *_comparator;
    
public:

    static const char* DEFAULT_STAGE_NAME;

    merge_stage_t()
        : _head_list(NULL)
    {
    }
    
protected:

    virtual result_t process_packet();
    
private:

    void insert_sorted(buffer_head_t *head);
};



#endif
