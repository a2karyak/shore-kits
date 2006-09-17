/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/tpch/drivers/tpch_q6.h"

#include "workload/common.h"
#include "core.h"

using namespace qpipe;

ENTER_NAMESPACE(workload);



void tpch_q6_driver::submit(void* disp) {
 
    scheduler::policy_t* dp = (scheduler::policy_t*)disp;
  
    packet_t* q6 =
        create_q6_packet( "Q6_CLIENT_", dp );
    guard<tuple_fifo> out = q6->output_buffer();
        
    dispatcher_t::dispatch_packet(q6);
    tuple_t output;
    while( out->get_tuple(output) ) {
        double* r = (double*)output.data;
        TRACE(TRACE_QUERY_RESULTS, "*** Q6 Count: %u. Sum: %lf.  ***\n", (unsigned)r[0], r[1]);
    }
}

EXIT_NAMESPACE(workload);
