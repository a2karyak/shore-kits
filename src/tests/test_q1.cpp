/* -*- mode:C++; c-basic-offset:4 -*- */

#include "tests/common/tpch_query.h"
#include "tests/common/register_stage.h"

#include "engine/stages/tscan.h"
#include "engine/stages/partial_aggregate.h"
#include "workload/tpch/drivers/tpch_q1.h"


int main(int argc, char* argv[]) {
    
    query_info_t info = query_init(argc, argv);
    
    register_stage<tscan_stage_t>(1);
    register_stage<partial_aggregate_stage_t>(1);
    tpch_q1_driver driver(c_str("Q1"));
                   
    query_main(info, &driver);
    return 0;
}
