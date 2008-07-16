/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_order_status_baseline.h
 *
 *  @brief Implementation for the Shore Baseline TPC-C Order_Status transaction
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "stages/tpcc/shore/shore_order_status_baseline.h"


using namespace qpipe;
using namespace shore;


const c_str shore_order_status_baseline_packet_t::PACKET_TYPE = "SHORE_ORDER_STATUS_BASELINE";

const c_str shore_order_status_baseline_stage_t::DEFAULT_STAGE_NAME = "SHORE_ORDER_STATUS_BASELINE_STAGE";


/**
 *  @brief shore_order_status_baseline_stage_t constructor
 */

shore_order_status_baseline_stage_t::shore_order_status_baseline_stage_t() 
{    
    TRACE(TRACE_DEBUG, "SHORE_ORDER_STATUS_BASELINE constructor\n");
}

/**
 *  @brief Execute TPC-C Order_Status transaction is a conventional way 
 *  using Shore underneath
 *
 *  @return void
 *
 *  @throw May throw exceptions on error.
 */

void shore_order_status_baseline_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;

    shore_order_status_baseline_packet_t* packet = 
	(shore_order_status_baseline_packet_t*)adaptor->get_packet();

    // Prints out the packet info
    //    packet->describe_trx();

    trx_result_tuple_t atrt;
    _g_shore_env->run_order_status(packet->get_trx_id(), packet->_os_in, atrt);

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





    
