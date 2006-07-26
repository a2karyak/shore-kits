/* -*- mode:C++; c-basic-offset:4 -*- */

#include "tests/common/tpch_query.h"
#include "tests/common/register_stage.h"

#include "engine/stages/tscan.h"
#include "engine/stages/hash_join.h"
#include "engine/stages/partial_aggregate.h"
#include "workload/tpch/drivers/tpch_q4.h"


int main(int argc, char* argv[]) {
    
    query_info_t info = query_init(argc, argv);
    
    // line up the stages...
    register_stage<tscan_stage_t>(2);
    register_stage<hash_join_stage_t>(1);
    register_stage<partial_aggregate_stage_t>(1);
    tpch_q4_driver driver(c_str("Q4"));
                   
    query_main(info, &driver);
    return 0;
}
