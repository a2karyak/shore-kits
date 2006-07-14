/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _BNL_JOIN_H
#define _BNL_JOIN_H

#include <cstdio>
#include <string>

#include "db_cxx.h"
#include "engine/core/tuple.h"
#include "engine/core/packet.h"
#include "engine/core/stage.h"
#include "engine/dispatcher.h"
#include "engine/tuple_source.h"
#include "trace.h"
#include "qpipe_panic.h"



using namespace std;
using namespace qpipe;



/* exported datatypes */


/**
 *  @brief
 */
struct bnl_join_packet_t : public packet_t {
  
    static const c_str PACKET_TYPE;

    pointer_guard_t<packet_t>       _left;
    buffer_guard_t                  _left_buffer;
    pointer_guard_t<tuple_source_t> _right_source;
    pointer_guard_t<tuple_join_t>   _join;
    
    
    /**
     *  @brief bnl_join_packet_t constructor.
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
     */
    bnl_join_packet_t(const char*     packet_id,
                      tuple_buffer_t* output_buffer,
                      tuple_filter_t* output_filter,
                      packet_t* left,
                      tuple_source_t* right_source,
                      tuple_join_t* join)
        : packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter, false),
          _left(left),
          _left_buffer(left->_output_buffer),
          _right_source(right_source),
          _join(join)
    {
    }
  
};



/**
 *  @brief Block-nested loop join stage.
 */
class bnl_join_stage_t : public stage_t {

public:

    static const c_str DEFAULT_STAGE_NAME;
    typedef bnl_join_packet_t stage_packet_t;
    
    bnl_join_stage_t() { }
    
    virtual ~bnl_join_stage_t() { }
    
protected:

    virtual void process_packet();
};



#endif
