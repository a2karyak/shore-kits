/* -*- mode:C++; c-basic-offset:4 -*- */

# include "scheduler.h"
# include "workload/tpcc/drivers/tpcc_payment.h"
# include "workload/common.h"


using namespace qpipe;

ENTER_NAMESPACE(workload);



void tpcc_payment_driver::submit(void* disp) {
 
    scheduler::policy_t* dp = (scheduler::policy_t*)disp;


    TRACE(TRACE_ALWAYS, "CREATING_PAYMENT_PACKET");
  
    trx_packet_t* payment = create_payment_packet( "PAYMENT_CLIENT_", dp );
    guard<tuple_fifo> out = payment->output_buffer();
        
    dispatcher_t::dispatch_packet(payment);
    tuple_t output;

    int row_counter = 0;
    while( out->get_tuple(output) ) {
        TRACE(TRACE_QUERY_RESULTS, "*** Count: %u.  ***\n",
	      ++row_counter);
    }
}

trx_packet_t* tpcc_payment_driver::create_payment_packet(const c_str &client_prefix, 
                                                         scheduler::policy_t* dp) 
{
    // BEGIN_PAYMENT
    payment_begin_packet_t* payment_packet;

    c_str packet_name("%s_payment_test", client_prefix.data());

    TRACE(TRACE_ALWAYS, "SHOULD CALL CORRECT payment_begin_packet_t CONSTRUCTOR!");
    payment_packet = new payment_begin_packet_t(packet_name,
						NULL,
						NULL,
						1,
						1,
						10,
						2,
						2,
						20,
						1,
						"EMPTY",
						5.0,
						"NEVER");
					       
    
    qpipe::query_state_t* qs = dp->query_state_create();
    payment_packet->assign_query_state(qs);

    return (payment_packet);
}

                                                

EXIT_NAMESPACE(workload);
