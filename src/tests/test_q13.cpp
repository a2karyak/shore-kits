/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages.h"
#include "tests/common/tpch_query.h"
#include "workload/common.h"

#include "workload/tpch/drivers/tpch_q13.h"


int main(int argc, char* argv[]) {
    
    query_info_t info = query_init(argc, argv);
    
    /* not sure how many partial_aggregate_t workers I need */
    register_stage<partial_aggregate_stage_t>(2);

    register_stage<tscan_stage_t>(2);
    register_stage<aggregate_stage_t>(2);
    register_stage<sort_stage_t>(3);
    register_stage<merge_stage_t>(10);
    register_stage<fdump_stage_t>(10);
    register_stage<fscan_stage_t>(20);
    register_stage<hash_join_stage_t>(2);
    workload::tpch_q13_driver driver(c_str("Q13"));
                   
    query_main(info, &driver);
    return 0;
}
