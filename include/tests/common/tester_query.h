/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __TESTER_QUERY_H
#define __TESTER_QUERY_H

#include "scheduler.h"

#include "workload/driver.h"
#include "workload/register_stage_containers.h"


// tracing the key comparison in tpcc
#define TESTER_TRACE_MASK (TRACE_ALWAYS | TRACE_STATISTICS | TRACE_QUERY_RESULTS | TRACE_KEY_COMP | TRACE_RECORD_FLOW | TRACE_TRX_FLOW )
//#define TESTER_TRACE_MASK (TRACE_ALWAYS | TRACE_STATISTICS | TRACE_QUERY_RESULTS | TRACE_RECORD_FLOW)

// default tracing flags
//#define TESTER_TRACE_MASK (TRACE_ALWAYS | TRACE_STATISTICS | TRACE_QUERY_RESULTS)


struct query_info_t {
    int num_iterations;
    scheduler::policy_t* _policy;
};


query_info_t query_init(int argc, char* argv[], int env = QUERY_ENV);


void query_main(query_info_t& info, workload::driver_t* driver, int env = QUERY_ENV);



#endif
