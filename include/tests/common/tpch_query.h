/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _TPCH_QUERY_H
#define _TPCH_QUERY_H

#include "engine/dispatcher/dispatcher_policy.h"
#include "workload/driver.h"

using namespace qpipe;


struct query_info_t {
    int num_iterations;
    dispatcher_policy_t* dispatcher_policy;
};


query_info_t query_init(int argc, char* argv[]);

void query_main(query_info_t& info, driver_t* driver);



#endif
