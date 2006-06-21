/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/thread.h"
#include "engine/core/stage_container.h"
#include "trace.h"
#include "qpipe_panic.h"



// include me last!
#include "engine/namespace.h"


/**
 *  @brief packet_t constructor.
 *
 *  @param packet_id The ID of this packet. This should be a block of
 *  bytes allocated with malloc(). This packet will take ownership of
 *  this block and invoke free() when it is destroyed.
 *
 *  @param packet_type The type of this packet. A packet DOES NOT own
 *  its packet_type string (we will not invoke delete or free() on
 *  this field in our packet destructor).
 *
 *  @param output_buffer The buffer where this packet should send its
 *  data. A packet DOES NOT own its output buffer (we will not invoke
 *  delete or free() on this field in our packet destructor).
 *
 *  @param output_filter The filter that will be applied to any tuple
 *  sent to _output_buffer. The packet OWNS this filter. It will be
 *  deleted in the packet destructor.
 *
 *  @param merge_enabled Whether this packet can be merged with other
 *  packets. This parameter is passed by value, so there is no
 *  question of ownership.
 */

packet_t::packet_t(char *          packet_id,
		   const char*     packet_type,
		   tuple_buffer_t* output_buffer,
		   tuple_filter_t* output_filter,
                   bool            merge_enabled)
    : _merge_enabled(merge_enabled),
      _packet_id(packet_id),
      _packet_type(packet_type),
      _output_buffer(output_buffer),
      _output_filter(output_filter),
      _next_tuple_on_merge(stage_container_t::NEXT_TUPLE_UNINITIALIZED),
      _next_tuple_needed  (stage_container_t::NEXT_TUPLE_INITIAL_VALUE)
{
    // error checking
    assert(packet_id     != NULL);
    assert(packet_type   != NULL);
    assert(output_buffer != NULL);
    assert(output_filter != NULL);

    TRACE(TRACE_PACKET_FLOW, "Created %s packet with ID %s\n",
	  _packet_type,
	  _packet_id);
}



/**
 *  @brief packet_t destructor.
 */

packet_t::~packet_t(void) {
    
    TRACE(TRACE_PACKET_FLOW, "Destroying %s packet with ID %s\n",
	  _packet_type,
	  _packet_id);

    // output buffer should have already been dealt with
    assert(_output_buffer == NULL);

    // we own our packet_id and our filter
    free(_packet_id);

    delete _output_filter;
}



#include "engine/namespace.h"
