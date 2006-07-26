/* -*- mode:C++; c-basic-offset:4 -*- */

#include "tests/common/tpch_query.h"
#include "tests/common/register_stage.h"

#include "engine/stages/tscan.h"
#include "engine/stages/partial_aggregate.h"
#include "engine/stages/hash_join.h"
#include "workload/tpch/drivers/tpch_q12.h"


int main(int argc, char* argv[]) {
    
    query_info_t info = query_init(argc, argv);
    
    register_stage<tscan_stage_t>();
    register_stage<partial_aggregate_stage_t>();
    register_stage<hash_join_stage_t>();
    tpch_q12_driver driver(c_str("Q12"));
                   
    query_main(info, &driver);
    return 0;
}
