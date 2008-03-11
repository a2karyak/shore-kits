/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_stock_level_baseline.h
 *
 *  @brief Implementation for the Shore Baseline TPC-C Stock_Level transaction
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "stages/tpcc/shore/shore_stock_level_baseline.h"


using namespace qpipe;
using namespace shore;


const c_str shore_stock_level_baseline_packet_t::PACKET_TYPE = "SHORE_STOCK_LEVEL_BASELINE";

const c_str shore_stock_level_baseline_stage_t::DEFAULT_STAGE_NAME = "SHORE_STOCK_LEVEL_BASELINE_STAGE";


/**
 *  @brief shore_stock_level_baseline_stage_t constructor
 */

shore_stock_level_baseline_stage_t::shore_stock_level_baseline_stage_t() 
{    
    TRACE(TRACE_DEBUG, "SHORE_STOCK_LEVEL_BASELINE constructor\n");
}

struct alloc_guard {
    char* _ptr;
    alloc_guard(int size) : _ptr((char*) pool()->alloc(size)) { }
    ~alloc_guard() { pool()->free(_ptr); }
    operator char*() { return _ptr; }
    DECLARE_POOL_ALLOC_POOL(alloc_guard);
};

/**
 *  @brief Execute TPC-C Stock_Level transaction is a conventional way 
 *  using Shore underneath
 *
 *  @return void
 *
 *  @throw May throw exceptions on error.
 */

void shore_stock_level_baseline_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;

    shore_stock_level_baseline_packet_t* packet = 
	(shore_stock_level_baseline_packet_t*)adaptor->get_packet();

    // Prints out the packet info
    //    packet->describe_trx();

    trx_result_tuple_t atrt;
    shore_env->run_stock_level(packet->get_trx_id(), packet->_sl_in, atrt);

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





    
