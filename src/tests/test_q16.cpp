/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages.h"
#include "tests/common/tpch_query.h"
#include "workload/common.h"

#include "workload/tpch/drivers/tpch_q16.h"


int main(int argc, char* argv[]) {

    query_info_t info = query_init(argc, argv);
    
    register_stage<tscan_stage_t>();
    register_stage<aggregate_stage_t>();
    register_stage<partial_aggregate_stage_t>();
    register_stage<sort_stage_t>();
    register_stage<merge_stage_t>();
    register_stage<fdump_stage_t>();
    register_stage<fscan_stage_t>(20);
    register_stage<hash_join_stage_t>();
    register_stage<sorted_in_stage_t>();

    workload::tpch_q16_driver driver(c_str("Q16"));
                   
    query_main(info, &driver);
    return 0;
}
