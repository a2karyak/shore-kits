/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _BNL_IN_H
#define _BNL_IN_H

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
 *  @brief This stage is used to implement IN/NOT IN using a
 *  block-nested loop.
 */
struct bnl_in_packet_t : public packet_t {
  
    static const char* PACKET_TYPE;

    pointer_guard_t<packet_t>        _left;
    pointer_guard_t<tuple_buffer_t>  _left_buffer;
    
    // the following fields should always be deleted by the destructor
    pointer_guard_t<tuple_source_t>  _right_source;
    pointer_guard_t<key_extractor_t> _extract;
    pointer_guard_t<key_compare_t>   _compare;
    bool _output_on_match;
    
    
    /**
     *  @brief bnl_in_packet_t constructor.
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
     */
    bnl_in_packet_t(char* packet_id,
                    tuple_buffer_t* output_buffer,
                    tuple_filter_t* output_filter,
                    packet_t* left,
                    tuple_source_t* right_source,
                    key_extractor_t* extract,
                    key_compare_t* compare,
                    bool output_on_match)
        : packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter, false),
          _left(left),
          _left_buffer(left->_output_buffer),
          _right_source(right_source),
          _extract(extract),
          _compare(compare),
          _output_on_match(output_on_match)
    {
    }
  
    virtual ~bnl_in_packet_t() {
        // do nothing
    }


    virtual void destroy_subpackets() {
        _left_buffer.done();
        _left->destroy_subpackets();
        _left.done();
    }
    
    
    virtual void terminate_inputs() {

        if ( _left_buffer->terminate() )
            // we have successfully given up ownership
            _left_buffer.release();
        // else, the producer has already terminated this buffer. We
        // are now responsible for deleting it.


        // TODO Ask the dispatcher to clear our left packet (_left)
        // from system, if it still exists.
        
        
        // _left is no longer in the system. Our destructor is not
        // responsible for freeing it.
        _left.release();
        
        
        // TODO destroy any other packets we may have dispatched
        // during our execution. Hopefully they will be destroyed in
        // process_packet() since we are using a tuple_source_t.
    }
};



/**
 *  @brief Block-nested loop stage for processing the IN/NOT IN
 *  operator.
 */
class bnl_in_stage_t : public stage_t {
    
public:
    
    static const char* DEFAULT_STAGE_NAME;
    typedef bnl_in_packet_t stage_packet_t;
    
    bnl_in_stage_t() { }
    
    virtual ~bnl_in_stage_t() { }
    
protected:

    virtual result_t process_packet();
};



#endif
