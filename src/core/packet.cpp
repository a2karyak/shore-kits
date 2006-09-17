/* -*- mode:C++; c-basic-offset:4 -*- */

#include "core/packet.h"
#include "core/stage_container.h"
#include "util.h"



ENTER_NAMESPACE(qpipe);

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

packet_t::packet_t(const c_str    &packet_id,
		   const c_str    &packet_type,
		   tuple_fifo*     output_buffer,
		   tuple_filter_t* output_filter,
                   query_plan*     plan,
                   bool            merge_enabled)
    : _plan(plan),
      _merge_enabled(merge_enabled),
      _packet_id(packet_id),
      _packet_type(packet_type),
      _output_buffer(output_buffer),
      _output_filter(output_filter),
      _next_tuple_on_merge(stage_container_t::NEXT_TUPLE_UNINITIALIZED),
      _next_tuple_needed  (stage_container_t::NEXT_TUPLE_INITIAL_VALUE),
      _cpu_bind(NULL)
{
    // error checking
    assert(output_buffer != NULL);
    assert(output_filter != NULL);
    assert(!merge_enabled || plan != NULL);

    TRACE(TRACE_PACKET_FLOW, "Created %s packet with ID %s\n",
	  _packet_type.data(),
	  _packet_id.data());
}



/**
 *  @brief packet_t destructor.
 */

packet_t::~packet_t(void) {
    
    TRACE(TRACE_PACKET_FLOW, "Destroying %s packet with ID %s\n",
	  _packet_type.data(),
	  _packet_id.data());
}



EXIT_NAMESPACE(qpipe);
