/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/tpch/drivers/tpch_q6.h"

#include "workload/common.h"
#include "core.h"

using namespace qpipe;

ENTER_NAMESPACE(workload);



void tpch_q6_driver::submit(void* disp) {
 
    scheduler::policy_t* dp = (scheduler::policy_t*)disp;
  
    thread_t* this_thread = thread_get_self();
    c_str qtag("Q6CL_%s_", this_thread->thread_name().data());
    packet_t* q6 = create_q6_packet( qtag.data(), dp );
    guard<tuple_fifo> out = q6->output_buffer();
        
    
    reserve_query_workers(q6);
    dispatcher_t::dispatch_packet(q6);


    tuple_t output;
    while( out->get_tuple(output) ) {
        decimal* r = aligned_cast<decimal>(output.data);
        TRACE(TRACE_QUERY_RESULTS, "*** Q6 Sum: %f. Count: %u.  ***\n",
	      r[0].to_double(), r[1].to_int());
    }
}

EXIT_NAMESPACE(workload);
