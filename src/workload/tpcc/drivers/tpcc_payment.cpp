/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_payment.cpp
 *
 *  @brief Implements client that submits BDB-PAYMENT transactions according to
 *   the TPC-C specification.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "scheduler.h"
#include "util.h"

#include "workload/common.h"

#include "workload/tpcc/drivers/tpcc_payment.h"


using namespace qpipe;
using namespace tpcc;


ENTER_NAMESPACE(workload);



void tpcc_payment_driver::submit(void* disp, memObject_t* mem) {

    // FIXME (ip) Should use the mem object.
    assert(mem);

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
    

    // payment_begin_packet
    bdb_trx_packet_t* bp_packet = 
	create_begin_payment_packet( "PAYMENT_CLIENT_", 
				     bp_buffer, 
				     bp_filter,
				     dp,
                                     QUERIED_TPCC_SCALING_FACTOR);


    qpipe::query_state_t* qs = dp->query_state_create();
    bp_packet->assign_query_state(qs);
    

    trx_result_process_tuple_t trx_rt(bp_packet);
    process_query(bp_packet, trx_rt);


    dp->query_state_destroy(qs);
}


/** @fn create_begin_payment_packet
 *
 *  @brief Creates a new PAYMENT request, given the scaling factor (sf) 
 *  of the database
 */

bdb_trx_packet_t* 
tpcc_payment_driver::create_begin_payment_packet(const c_str &client_prefix, 
						 tuple_fifo* bp_output_buffer,
						 tuple_filter_t* bp_output_filter,
						 scheduler::policy_t* dp,
                                                 int sf) 
{
    assert(sf > 0);

    tpcc::payment_input_t pin = create_payment_input(sf);
    
    c_str packet_name("%s_payment_test", client_prefix.data());


    bdb_trx_packet_t* payment_packet 
        = new payment_begin_packet_t(packet_name,
                                     bp_output_buffer,
                                     bp_output_filter,
                                     pin._home_wh_id,
                                     pin._home_d_id,
                                     pin._v_cust_wh_selection,
                                     pin._remote_wh_id,
                                     pin._remote_d_id,
                                     pin._v_cust_ident_selection,
                                     pin._c_id,
                                     pin._c_last,
                                     pin._h_amount,
                                     pin._h_date);
    
    qpipe::query_state_t* qs = dp->query_state_create();
    payment_packet->assign_query_state(qs);

    return (payment_packet);
}

                                                

EXIT_NAMESPACE(workload);
