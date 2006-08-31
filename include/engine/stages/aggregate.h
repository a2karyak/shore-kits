/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __AGGREGATE_H
#define __AGGREGATE_H

#include <cstdio>

#include "engine/core/tuple.h"
#include "engine/core/packet.h"
#include "engine/core/stage.h"
#include "trace.h"
#include "qpipe_panic.h"



using namespace qpipe;



/**
 *  @brief Packet definition for the aggregation stage.
 */

struct aggregate_packet_t : public packet_t {

    static const c_str PACKET_TYPE;
    
    guard<tuple_aggregate_t> _aggregator;
    guard<key_extractor_t>   _extract;
    guard<packet_t>          _input;
    guard<tuple_fifo>        _input_buffer;
    
    
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
     *  @param aggregator The aggregator we will be using for this
     *  packet. The packet OWNS this aggregator. It will be deleted in
     *  the packet destructor.
     *
     *  @param input The input packet for this aggregator. The packet
     *  takes ownership of this packet, but it will hand off ownership
     *  to a container as soon as this packet is dispatched.
     */
    aggregate_packet_t(const c_str       &packet_id,
		       tuple_fifo*        output_buffer,
		       tuple_filter_t*    output_filter,
		       tuple_aggregate_t* aggregator,
                       key_extractor_t*   extract,
                       packet_t*          input)
	: packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter,
                   create_plan(output_filter, aggregator, extract, input->plan())),
	  _aggregator(aggregator), _extract(extract),
          _input(input),
          _input_buffer(input->output_buffer())
    {
        assert(_input != NULL);
        assert(_input_buffer != NULL);
    }

    static query_plan* create_plan(tuple_filter_t* filter, tuple_aggregate_t* agg,
                                   key_extractor_t* key, query_plan const* child)
    {
        c_str action("%s:%s:%s", PACKET_TYPE.data(),
                     agg->to_string().data(), key->to_string().data());

        query_plan const** children = new query_plan const*[1];
        children[0] = child;
        return new query_plan(action, filter->to_string(), children, 1);
    }
    
};



/**
 *  @brief Aggregation stage that aggregates over grouped inputs. It
 *  produces one output tuple for each input set of tuples.
 */

class aggregate_stage_t : public stage_t {

public:

    static const c_str DEFAULT_STAGE_NAME;
    typedef aggregate_packet_t stage_packet_t;

    aggregate_stage_t() { }

    ~aggregate_stage_t() { }


 protected:

    virtual void process_packet();
};



#endif
