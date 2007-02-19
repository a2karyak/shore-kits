/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages.h"
#include "tests/common/tester_query.h"
#include "workload/common.h"

#include "workload/tpch/drivers/tpch_q6.h"


int main(int argc, char* argv[]) {

    query_info_t info = query_init(argc, argv);
    
    register_stage<tscan_stage_t>(1);
    register_stage<aggregate_stage_t>(1);
    workload::tpch_q6_driver driver(c_str("Q6"));

    trace_set(TRACE_QUERY_RESULTS);

    query_main(info, &driver);
    return 0;
}
