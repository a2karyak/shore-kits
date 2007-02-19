/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _TESTER_QUERY_H
#define _TESTER_QUERY_H

#include "scheduler.h"
#include "workload/driver.h"


struct query_info_t {
    int num_iterations;
    scheduler::policy_t* _policy;
};


query_info_t query_init(int argc, char* argv[]);

void query_main(query_info_t& info, workload::driver_t* driver);



#endif
