/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file payment_baseline.h
 *
 *  @brief Implementation for the Baseline TPC-C Payment transaction
 *
 *  @author Ippokratis Pandis (ipandis)
 */


# include "stages/tpcc/payment_baseline.h"
# include "workload/common.h"


const c_str payment_baseline_packet_t::PACKET_TYPE = "PAYMENT_BASELINE";

const c_str payment_baseline_stage_t::DEFAULT_STAGE_NAME = "PAYMENT_BASELINE_STAGE";


/**
 *  @brief payment_baseline_stage_t constructor
 */

payment_baseline_stage_t::payment_baseline_stage_t() {
    
    TRACE(TRACE_ALWAYS, "PAYMENT_BASELINE constructor\n");
}


/**
 *  @brief Execute TPC-C Payment transaction is a conventional way
 *
 *  @return void
 *
 *  @throw May throw exceptions on error.
 */

void payment_baseline_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;

    payment_begin_packet_t* packet = 
	(payment_begin_packet_t*)adaptor->get_packet();

    // Prints out the packet info
    packet->describe_trx();

    // Executes trx

    TRACE( TRACE_ALWAYS, "*** EXECUTING TRX CONVENTIONALLY ***\n");

    
    TRACE( TRACE_ALWAYS, "DONE. NOTIFYING CLIENT\n" );
    
    // writing output 

    // create output tuple
    // "I" own tup, so allocate space for it in the stack
    size_t dest_size = packet->output_buffer()->tuple_size();
    
    array_guard_t<char> dest_data = new char[dest_size];
    tuple_t dest(dest_data, dest_size);
    
    // FIXME: Sending always COMMITTED
    trx_result_tuple aTrxResultTuple(COMMITTED, packet->get_trx_id());
    
    trx_result_tuple* dest_result_tuple;
    dest_result_tuple = aligned_cast<trx_result_tuple>(dest.data);
    
    *dest_result_tuple = aTrxResultTuple;
    
    adaptor->output(dest);


} // process_packet
