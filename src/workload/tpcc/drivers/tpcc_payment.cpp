/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_payment.cpp
 *
 *  @brief Implements client that submits PAYMENT transaction according to the TPCC
 *  specification.
 */

# include "scheduler.h"
# include "util.h"

# include "workload/common.h"

# include "workload/tpcc/drivers/common.h"
# include "workload/tpcc/drivers/tpcc_payment.h"


using namespace qpipe;

ENTER_NAMESPACE(workload);


/** FIXME: (ip) SF constant */
const int SF = 10;


class payment_process_tuple_t : public process_tuple_t {

private:

    int row_counter;

public:

    payment_process_tuple_t()
        : row_counter(0)
    {
    }
        
    virtual void begin() {
        TRACE(TRACE_QUERY_RESULTS, "*** PAYMENT ANSWER ...\n");
    }
    
    virtual void process(const tuple_t& output) {
	int* tmp;
	tmp = aligned_cast<int>(output.data);
        TRACE(TRACE_QUERY_RESULTS, "*** Count: %d. TRX: %d ***\n",
	      ++row_counter, *tmp);
    }

}; // END OF: payment_process_tuple_t


void tpcc_payment_driver::submit(void* disp) {
 
    scheduler::policy_t* dp = (scheduler::policy_t*)disp;


    // payment_begin_packet output
    // FIXME: (ip) I don't know if we need output buffers for the PAYMENT_BEGIN
    //             but the code asserts if output_buffer == NULL
    tuple_fifo* bp_buffer = new tuple_fifo(sizeof(int));


    // payment_begin_packet filter
    // FIXME: (ip) I also don't believe that we need filters for the PAYMENT
    //             but the code asserts if output_filter == NULL
    tuple_filter_t* bp_filter = new trivial_filter_t(sizeof(int));
    

    // payment_begin_packet
    payment_begin_packet_t* bp_packet = 
	create_begin_payment_packet( "PAYMENT_CLIENT_", 
				     bp_buffer, 
				     bp_filter,
				     dp,
                                     SF);


    qpipe::query_state_t* qs = dp->query_state_create();
    bp_packet->assign_query_state(qs);
    

    payment_process_tuple_t pt;
    process_query(bp_packet, pt);


    dp->query_state_destroy(qs);
}


/** @func create_begin_payment_packet
 *
 *  @brief Creates a new PAYMENT request, given the scaling factor (sf) of the database
 */

payment_begin_packet_t* 
tpcc_payment_driver::create_begin_payment_packet(const c_str &client_prefix, 
						 tuple_fifo* bp_output_buffer,
						 tuple_filter_t* bp_output_filter,
						 scheduler::policy_t* dp,
                                                 int sf) 
{
    // BEGIN_PAYMENT
    payment_begin_packet_t* payment_packet;

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

    char* p_date = "NEVER";
    
    c_str packet_name("%s_payment_test", client_prefix.data());


    TRACE(TRACE_ALWAYS, 
	  "!! HARD-CODED payment_begin_packet_t\n");




    payment_packet = new payment_begin_packet_t(packet_name,
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
					       
    
    qpipe::query_state_t* qs = dp->query_state_create();
    payment_packet->assign_query_state(qs);

    payment_packet->describe_trx();

    return (payment_packet);
}

                                                

EXIT_NAMESPACE(workload);
