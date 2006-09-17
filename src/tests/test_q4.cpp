/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages.h"
#include "tests/common/tpch_query.h"
#include "workload/common.h"
#include "workload/tpch/drivers/tpch_q4.h"


int main(int argc, char* argv[]) {
    
    query_info_t info = query_init(argc, argv);
    
    // line up the stages...
    register_stage<tscan_stage_t>(2);
    register_stage<hash_join_stage_t>(1);
    register_stage<partial_aggregate_stage_t>(1);
    workload::tpch_q4_driver driver(c_str("Q4"));
                   
    query_main(info, &driver);
    return 0;
}
