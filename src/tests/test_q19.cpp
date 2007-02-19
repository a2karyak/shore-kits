/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages.h"
#include "tests/common/tester_query.h"
#include "workload/common.h"

#include "workload/tpch/drivers/tpch_q19.h"


int main(int argc, char* argv[]) {
    
    query_info_t info = query_init(argc, argv);
    
    /* not sure how many partial_aggregate_t workers I need */
    // line up the stages...
    register_stage<tscan_stage_t>();
    register_stage<aggregate_stage_t>();
    register_stage<hash_join_stage_t>();
    workload::tpch_q19_driver driver(c_str("Q19"));
                   
    query_main(info, &driver);
    return 0;
}
