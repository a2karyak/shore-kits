/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/tpch/drivers/tpch_q6.h"

#include "workload/common.h"
#include "engine/dispatcher.h"

using namespace qpipe;



void tpch_q6_driver::submit(void* disp) {
 
    dispatcher_policy_t* dp = (dispatcher_policy_t*)disp;
  
    packet_t* q6 =
        create_q6_packet( "Q6_CLIENT_", dp );
    tuple_buffer_t* out = q6->_output_buffer;
        
    dispatcher_t::dispatch_packet(q6);
    tuple_t output;
    while( out->get_tuple(output) ) {
        double* r = (double*)output.data;
        TRACE(TRACE_QUERY_RESULTS, "*** Q6 Count: %u. Sum: %lf.  ***\n", (unsigned)r[0], r[1]);
    }
}
