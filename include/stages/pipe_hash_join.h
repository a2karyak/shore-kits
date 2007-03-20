/* -*- mode:C++; c-basic-offset:4 -*- */

/* pipe_hash_join_stage.h */
/* Declaration of the pipe_hash_join_stage pipe_hash_join_packet classes */
/* History: 
   3/3/2006: Removed the static pipe_hash_join_cnt variable of the pipe_hash_join_packet_t class, instead it  uses the singleton stage_packet_counter class.
*/


#ifndef __PIPE_HASH_JOIN_STAGE_H
#define __PIPE_HASH_JOIN_STAGE_H

#include "core.h"

#include <string>
using namespace qpipe;
using std::string;
using std::vector;


#define PIPE_HASH_JOIN_STAGE_NAME  "PIPE_HASH_JOIN"
#define PIPE_HASH_JOIN_PACKET_TYPE "PIPE_HASH_JOIN"


/********************
 * pipe_hash_join_packet *
 ********************/


class pipe_hash_join_packet_t : public packet_t {

public:
    static const c_str PACKET_TYPE;

    guard<packet_t> _left;
    guard<packet_t> _right;
    guard<tuple_fifo> _left_buffer;
    guard<tuple_fifo> _right_buffer;

    guard<tuple_join_t> _join;
    
    int count_out;
    int count_left;
    int count_right;
  
    /**
     *  @brief Constructor.
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
     *  @param left Left side-input packet. This will be dispatched
     *  second and become the outer relation of the join. This should
     *  be the larger input.
     *
     *  @param right Right-side packet. This will be dispatched first
     *  and will become the inner relation of the join. It should be
     *  the smaller input.
     *
     *  @param join The joiner  we will be using for this
     *  packet. The packet OWNS this joiner. It will be deleted in
     *  the packet destructor.
     *
     */
    pipe_hash_join_packet_t(const c_str &packet_id,
			    tuple_fifo* out_buffer,
			    tuple_filter_t *output_filter,
			    packet_t* left,
			    packet_t* right,
			    tuple_join_t *join)

        : packet_t(packet_id, PACKET_TYPE, out_buffer, output_filter,
                   create_plan(output_filter, join, left, right),
                   true, /* merging allowed */
                   true  /* unreserve worker on completion */
                   ),
          _left(left),
          _right(right),
          _left_buffer(left->output_buffer()),
          _right_buffer(right->output_buffer()),
          _join(join)
    {
    }
  
    static query_plan* create_plan(tuple_filter_t* filter, tuple_join_t* join,
                                   packet_t* left, packet_t* right)
    {
        c_str action("%s:%s", PACKET_TYPE.data(),
                     join->to_string().data());

        query_plan const** children = new query_plan const*[2];
        children[0] = left->plan();
        children[1] = right->plan();
        return new query_plan(action, filter->to_string(), children, 2);
    }

    virtual void declare_worker_needs(resource_declare_t* declare) {
        declare->declare(_packet_type, 1);
        _left->declare_worker_needs(declare);
        _right->declare_worker_needs(declare);
    }
};



/*******************
 * pipe_hash_join_stage *
 *******************/

class pipe_hash_join_stage_t : public stage_t {


private:


    
    /* fields */
    
    tuple_join_t *_join;



public:

    typedef pipe_hash_join_packet_t stage_packet_t;

    static const c_str DEFAULT_STAGE_NAME;


    virtual void process_packet();
    
    
};



#endif	// __PIPE_HASH_JOIN_H
