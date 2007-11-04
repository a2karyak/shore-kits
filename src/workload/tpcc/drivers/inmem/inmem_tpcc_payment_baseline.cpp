/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file inmem_tpcc_payment_baseline.cpp
 *
 *  @brief Implements client that submits INMEM_PAYMENT_BASELINE transaction requests,
 *  according to the TPCC specification.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "scheduler.h"
#include "util.h"
#include "workload/common.h"
#include "workload/tpcc/drivers/inmem/inmem_tpcc_payment_baseline.h"


using namespace qpipe;
using namespace tpcc;
using namespace tpcc_payment;


ENTER_NAMESPACE(workload);


void inmem_tpcc_payment_baseline_driver::submit(void* disp, memObject_t*) {
 
    scheduler::policy_t* dp = (scheduler::policy_t*)disp;

    // payment_begin_packet output
    // FIXME: (ip) I don't know if we need output buffers for the PAYMENT_BEGIN
    //             but the code asserts if output_buffer == NULL
    tuple_fifo* bp_buffer = new tuple_fifo(sizeof(trx_result_tuple_t));

    // payment_begin_packet filter
    // FIXME: (ip) I also don't believe that we need filters for the PAYMENT
    //             but the code asserts if output_filter == NULL

    // (ip) size of output_filter and output_buffer should be the same!
    tuple_filter_t* bp_filter = 
        new trivial_filter_t(sizeof(trx_result_tuple_t));
    

    // payment_baseline_packet
    trx_packet_t* bp_packet = 
	create_inmem_payment_baseline_packet( "INMEM_PAYMENT_BASELINE_CLIENT_", 
                                              bp_buffer, 
                                              bp_filter,
                                              dp,
                                              selectedQueriedSF);
    
    qpipe::query_state_t* qs = dp->query_state_create();
    bp_packet->assign_query_state(qs);
        
    trx_result_process_tuple_t trx_rt(bp_packet);
    process_query(bp_packet, trx_rt);


    dp->query_state_destroy(qs);
}


/** @fn create_inmem_payment_baseline_packet
 *
 *  @brief Creates a new INMEM_PAYMENT_BASELINE request, given the scaling factor (sf) 
 *  of the database
 */

trx_packet_t* 
inmem_tpcc_payment_baseline_driver::
create_inmem_payment_baseline_packet(const c_str &client_prefix, 
                                     tuple_fifo* bp_output_buffer,
                                     tuple_filter_t* bp_output_filter,
                                     scheduler::policy_t* dp,
                                     int sf) 
{
    assert(sf>0);

    trx_packet_t* payment_packet;

    tpcc::payment_input_t pin = create_payment_input(sf);
    
    c_str packet_name("%s_payment_test", client_prefix.data());

    payment_packet = new inmem_payment_baseline_packet_t(packet_name,
                                                         bp_output_buffer,
                                                         bp_output_filter,
                                                         pin);
    
    qpipe::query_state_t* qs = dp->query_state_create();
    payment_packet->assign_query_state(qs);

    return (payment_packet);
}

                                                

EXIT_NAMESPACE(workload);
