/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages/func_call.h"
#include "qpipe_panic.h"
#include "trace/trace.h"
#include "tuple.h"



const char* func_call_packet_t::PACKET_TYPE = "FUNC_CALL";



const char* func_call_stage_t::DEFAULT_STAGE_NAME = "FUNC_CALL_STAGE";



/**
 *  @brief Invoke a packet's specified function with the packet's
 *  tuple buffer.
 *
 *  @return 0 on success. Non-zero on unrecoverable error. The stage
 *  should terminate all queries it is processing.
 */

stage_t::result_t func_call_stage_t::process_packet() {
    
    adaptor_t* adaptor = _adaptor;
    func_call_packet_t* packet = (func_call_packet_t*)adaptor->get_packet();
    return packet->_func(packet->_func_arg);
}
