/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file inmem_payment_baseline.h
 *
 *  @brief Implementation for the InMemory Baseline TPC-C Payment transaction
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "stages/tpcc/inmem/inmem_payment_baseline.h"
#include "stages/tpcc/common/payment_functions.h"

#include "workload/tpcc/tpcc_env.h"

using namespace qpipe;
using namespace tpcc_payment;


const c_str inmem_payment_baseline_packet_t::PACKET_TYPE = "INMEM_PAYMENT_BASELINE";

const c_str inmem_payment_baseline_stage_t::DEFAULT_STAGE_NAME = "INMEM_PAYMENT_BASELINE_STAGE";


/**
 *  @brief inmem_payment_baseline_stage_t constructor
 */

inmem_payment_baseline_stage_t::inmem_payment_baseline_stage_t() {
    
    TRACE(TRACE_DEBUG, "INMEM_PAYMENT_BASELINE constructor\n");
}


/**
 *  @brief Execute TPC-C Payment transaction is a conventional way 
 *  with in-memory data structures
 *
 *  @return void
 *
 *  @throw May throw exceptions on error.
 */

void inmem_payment_baseline_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;

    inmem_payment_baseline_packet_t* packet = 
	(inmem_payment_baseline_packet_t*)adaptor->get_packet();

    // Prints out the packet info
    packet->describe_trx();

    trx_result_tuple_t aTrxResultTuple = executeInMemPaymentBaseline(&packet->_p_in,
                                                                     packet->get_trx_id());
    

    TRACE( TRACE_TRX_FLOW, "DONE. NOTIFYING CLIENT\n" );
    
    // writing output 

    // create output tuple
    // "I" own tup, so allocate space for it in the stack
    size_t dest_size = packet->output_buffer()->tuple_size();
    
    array_guard_t<char> dest_data = new char[dest_size];
    tuple_t dest(dest_data, dest_size);
    
    trx_result_tuple_t* dest_result_tuple;
    dest_result_tuple = aligned_cast<trx_result_tuple_t>(dest.data);
    
    *dest_result_tuple = aTrxResultTuple;
    
    adaptor->output(dest);


} // process_packet





    
