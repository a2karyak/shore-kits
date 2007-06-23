/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_payment_baseline.cpp
 *
 *  @brief Implements client that submits PAYMENT_BASELINE transaction requests,
 *  according to the TPCC specification.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

# include "scheduler.h"
# include "util.h"

# include "workload/common.h"

# include "workload/tpcc/drivers/common.h"
# include "workload/tpcc/drivers/tpcc_payment_baseline.h"


using namespace qpipe;

ENTER_NAMESPACE(workload);


void tpcc_payment_baseline_driver::submit(void* disp) {
 
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
	create_payment_baseline_packet( "PAYMENT_BASELINE_CLIENT_", 
                                        bp_buffer, 
                                        bp_filter,
                                        dp,
                                        whRange);
    
    qpipe::query_state_t* qs = dp->query_state_create();
    bp_packet->assign_query_state(qs);
    
    
    trx_result_process_tuple_t trx_rt(bp_packet);
    process_query(bp_packet, trx_rt);


    dp->query_state_destroy(qs);
}


/** @fn create_payment_baseline_packet
 *
 *  @brief Creates a new PAYMENT_BASELINE request, given the scaling factor (sf) 
 *  of the database
 */

trx_packet_t* 
tpcc_payment_baseline_driver::create_payment_baseline_packet(const c_str &client_prefix, 
                                                             tuple_fifo* bp_output_buffer,
                                                             tuple_filter_t* bp_output_filter,
                                                             scheduler::policy_t* dp,
                                                             int sf) 
{
    trx_packet_t* payment_packet;

    assert(sf > 0);

    // produce PAYMENT params according to tpcc spec v.5.4
    int p_home_wh_id = URand(1, sf);
    int p_home_d_id = URand(1, 10);
    int select_wh = URand(1, 100); // 85 - 15
    int p_rem_wh_id = URand(1, sf);
    int p_rem_d_id = URand(1, 10);
    int select_ident = URand(1, 100); // 60 - 40
    int p_c_id = NURand(1023, 1, 3000);
    char* p_c_last = generate_cust_last(NURand(255, 0, 999));
    long p_amount = (long)URand(100, 500000)/(long)100.00;

    // FIXME: (ip) Correct p_date value
    char* p_date = "NEVER";
    
    c_str packet_name("%s_payment_test", client_prefix.data());



    payment_packet = new payment_baseline_packet_t(packet_name,
                                                   bp_output_buffer,
                                                   bp_output_filter,
                                                   p_home_wh_id,
                                                   p_home_d_id,
                                                   select_wh,
                                                   p_rem_wh_id,
                                                   p_rem_d_id,
                                                   select_ident,
                                                   p_c_id,
                                                   p_c_last,
                                                   p_amount,
                                                   p_date);

    payment_packet->describe_trx();
    
    qpipe::query_state_t* qs = dp->query_state_create();
    payment_packet->assign_query_state(qs);

    return (payment_packet);
}

                                                

EXIT_NAMESPACE(workload);
