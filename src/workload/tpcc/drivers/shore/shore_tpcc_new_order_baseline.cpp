/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_tpcc_new_order_baseline.cpp
 *
 *  @brief Implements client that submits SHORE_NEW_ORDER_BASELINE transaction requests,
 *  according to the TPCC specification.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "scheduler.h"
#include "util.h"
#include "stages/common.h"
#include "workload/tpcc/drivers/shore/shore_tpcc_new_order_baseline.h"


using namespace qpipe;
using namespace tpcc;


ENTER_NAMESPACE(workload);


void shore_tpcc_new_order_baseline_driver::submit(void* disp) {
 
    scheduler::policy_t* dp = (scheduler::policy_t*)disp;

    // new_order_begin_packet output
    // FIXME: (ip) I don't know if we need output buffers for the NEW_ORDER_BEGIN
    //             but the code asserts if output_buffer == NULL
    tuple_fifo* bp_buffer = new tuple_fifo(sizeof(trx_result_tuple_t));

    // new_order_begin_packet filter
    // FIXME: (ip) I also don't believe that we need filters for the NEW_ORDER
    //             but the code asserts if output_filter == NULL

    // (ip) size of output_filter and output_buffer should be the same!
    tuple_filter_t* bp_filter = 
        new trivial_filter_t(sizeof(trx_result_tuple_t));
    

    // new_order_baseline_packet
    trx_packet_t* bp_packet = 
	create_shore_new_order_baseline_packet(bp_buffer, 
                                               bp_filter,
                                               shore_env->get_qf());
    
    qpipe::query_state_t* qs = dp->query_state_create();
    bp_packet->assign_query_state(qs);
        
    trx_result_process_tuple_t trx_rt(bp_packet);
    process_query(bp_packet, trx_rt);

    dp->query_state_destroy(qs);
}


/** @fn create_shore_new_order_baseline_packet
 *
 *  @brief Creates a new SHORE_NEW_ORDER_BASELINE request, given the scaling factor (sf) 
 *  of the database
 */

trx_packet_t* 
shore_tpcc_new_order_baseline_driver::
create_shore_new_order_baseline_packet(tuple_fifo* bp_output_buffer,
                                       tuple_filter_t* bp_output_filter,
                                       int sf) 
{
    assert(sf>0);

    trx_packet_t* new_order_packet;

    tpcc::new_order_input_t noin = create_no_input(sf);
    
    static c_str packet_name = "SHORE_NEW_ORDER_BASELINE_CLIENT";

    new_order_packet = new shore_new_order_baseline_packet_t(packet_name,
                                                             bp_output_buffer,
                                                             bp_output_filter,
                                                             noin);
    
    return (new_order_packet);
}

                                                

EXIT_NAMESPACE(workload);
