/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __TESTER_QUERY_H
#define __TESTER_QUERY_H

#include "scheduler.h"

#include "workload/driver.h"
#include "workload/register_stage_containers.h"


/** Tracing flags for the tests */

// tracing the key comparison in tpcc
//#define TESTER_TRACE_MASK (TRACE_ALWAYS | TRACE_STATISTICS | TRACE_QUERY_RESULTS | TRACE_KEY_COMP | TRACE_RECORD_FLOW | TRACE_TRX_FLOW )
#define TESTER_TRACE_MASK (TRACE_ALWAYS | TRACE_STATISTICS | TRACE_QUERY_RESULTS | TRACE_RECORD_FLOW | TRACE_TRX_FLOW )

// default tracing flags
#define DEFAULT_TRACE_MASK (TRACE_ALWAYS | TRACE_STATISTICS | TRACE_QUERY_RESULTS)

// tracing flags for the db loads
#define LOAD_TRACE_MASK (TRACE_ALWAYS | TRACE_STATISTICS )


/** Exported data structures */


struct query_info_t {
    int num_iterations;
    scheduler::policy_t* _policy;
    int num_clients;
    int scaling_factor;
};


/** Exported functions */

// Staged execution
query_info_t query_init(int argc, char* argv[], int env = TRX_SHORE_ENV);
void query_main(query_info_t& info, workload::driver_t* driver, 
                int env = TRX_SHORE_ENV);

// Conventional (single-thread per request) execution
query_info_t query_single_thr_init(int argc, char* argv[], int env);






#endif
