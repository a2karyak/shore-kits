/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_delivery_baseline.h
 *
 *  @brief Implementation for the Shore Baseline TPC-C Delivery transaction
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "stages/tpcc/shore/shore_delivery_baseline.h"


using namespace qpipe;
using namespace tpcc;


const c_str shore_delivery_baseline_packet_t::PACKET_TYPE = "SHORE_DELIVERY_BASELINE";

const c_str shore_delivery_baseline_stage_t::DEFAULT_STAGE_NAME = "SHORE_DELIVERY_BASELINE_STAGE";


/**
 *  @brief shore_delivery_baseline_stage_t constructor
 */

shore_delivery_baseline_stage_t::shore_delivery_baseline_stage_t() 
{    
    TRACE(TRACE_DEBUG, "SHORE_DELIVERY_BASELINE constructor\n");
}

/**
 *  @brief Execute TPC-C Delivery transaction is a conventional way 
 *  using Shore underneath
 *
 *  @return void
 *
 *  @throw May throw exceptions on error.
 */

void shore_delivery_baseline_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;

    shore_delivery_baseline_packet_t* packet = 
	(shore_delivery_baseline_packet_t*)adaptor->get_packet();

    // Prints out the packet info
    //    packet->describe_trx();

    trx_result_tuple_t atrt;
    _g_shore_env->run_delivery(packet->get_trx_id(), packet->_d_in, atrt);

    TRACE( TRACE_TRX_FLOW, "DONE. NOTIFYING CLIENT\n" );
    
    // writing output 

    // create output tuple
    // "I" own tup, so allocate space for it in the stack
    size_t dest_size = packet->output_buffer()->tuple_size();
    
    alloc_guard dest_data = dest_size;
    tuple_t dest(dest_data, dest_size);
    
    trx_result_tuple_t* dest_result_tuple;
    dest_result_tuple = aligned_cast<trx_result_tuple_t>(dest.data);
    
    *dest_result_tuple = atrt;
    
    adaptor->output(dest);

} // process_packet





    
