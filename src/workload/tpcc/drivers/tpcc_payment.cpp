/* -*- mode:C++; c-basic-offset:4 -*- */

# include "scheduler.h"
# include "workload/tpcc/drivers/tpcc_payment.h"
# include "workload/common.h"



using namespace qpipe;

ENTER_NAMESPACE(workload);



using namespace qpipe;


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


    TRACE(TRACE_ALWAYS, "CREATING_BEGIN_PAYMENT_PACKET\n");


    // payment_begin_packet output
    // FIXME: ip I don't know if we need output buffers for the PAYMENT_BEGIN
    //           but the code asserts if output_buffer == NULL
    tuple_fifo* bp_buffer = new tuple_fifo(sizeof(int));


    // payment_begin_packet filter
    // FIXME: ip I also don't believe that we need filters for the PAYMENT
    //           but the code asserts if output_filter == NULL
    tuple_filter_t* bp_filter = new trivial_filter_t(sizeof(int));
    

    // payment_begin_packet
    payment_begin_packet_t* bp_packet = 
	create_begin_payment_packet( "PAYMENT_CLIENT_", 
				     bp_buffer, 
				     bp_filter,
				     dp );


    qpipe::query_state_t* qs = dp->query_state_create();
    bp_packet->assign_query_state(qs);
    

    payment_process_tuple_t pt;
    process_query(bp_packet, pt);


    dp->query_state_destroy(qs);
}


payment_begin_packet_t* 
tpcc_payment_driver::create_begin_payment_packet(const c_str &client_prefix, 
						 tuple_fifo* bp_output_buffer,
						 tuple_filter_t* bp_output_filter,
						 scheduler::policy_t* dp) 
{
    // BEGIN_PAYMENT
    payment_begin_packet_t* payment_packet;

    c_str packet_name("%s_payment_test", client_prefix.data());


    TRACE(TRACE_ALWAYS, 
	  "HARD-CODED payment_begin_packet_t\n");

    char* param_last = "EMPTY";
    char* param_date = "NEVER";

    payment_packet = new payment_begin_packet_t(packet_name,
						bp_output_buffer,
						bp_output_filter,
						1,
						1,
						10,
						2,
						2,
						20,
						1,
						param_last,
						5.0,
						param_date);
					       
    
    qpipe::query_state_t* qs = dp->query_state_create();
    payment_packet->assign_query_state(qs);

    return (payment_packet);
}

                                                

EXIT_NAMESPACE(workload);
