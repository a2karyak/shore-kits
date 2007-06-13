/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _TESTER_QUERY_H
#define _TESTER_QUERY_H

#include "scheduler.h"

#include "workload/driver.h"
#include "workload/register_stage_containers.h"


struct query_info_t {
    int num_iterations;
    scheduler::policy_t* _policy;
};


query_info_t query_init(int argc, char* argv[], int env = QUERY_ENV);


void query_main(query_info_t& info, workload::driver_t* driver, int env = QUERY_ENV);



#endif
