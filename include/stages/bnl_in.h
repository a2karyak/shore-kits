/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _BNL_IN_H
#define _BNL_IN_H

#include <cstdio>
#include <string>

#include "core.h"
#include "stages/tuple_source.h"



using namespace std;
using namespace qpipe;



/* exported datatypes */


/**
 *  @brief This stage is used to implement IN/NOT IN using a
 *  block-nested loop.
 */
struct bnl_in_packet_t : public packet_t {
  
    static const c_str PACKET_TYPE;

    guard<packet_t>        _left;
    guard<tuple_fifo>      _left_buffer;
    
    // the following fields should always be deleted by the destructor
    guard<tuple_source_t>  _right_source;
    guard<key_extractor_t> _extract;
    guard<key_compare_t>   _compare;
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
    bnl_in_packet_t(const c_str    &packet_id,
                    tuple_fifo* output_buffer,
                    tuple_filter_t* output_filter,
                    packet_t*       left,
                    tuple_source_t* right_source,
                    key_extractor_t* extract,
                    key_compare_t*  compare,
                    bool            output_on_match)
        : packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter, NULL, false),
          _left(left),
          _left_buffer(left->output_buffer()),
          _right_source(right_source),
          _extract(extract),
          _compare(compare),
          _output_on_match(output_on_match)
    {
    }
  
};



/**
 *  @brief Block-nested loop stage for processing the IN/NOT IN
 *  operator.
 */
class bnl_in_stage_t : public stage_t {
    
public:
    
    static const c_str DEFAULT_STAGE_NAME;
    typedef bnl_in_packet_t stage_packet_t;
    
    bnl_in_stage_t() { }
    
    virtual ~bnl_in_stage_t() { }
    
protected:

    virtual void process_packet();
};



#endif
