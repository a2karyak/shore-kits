/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages.h"
#include "workload/common/register_stage.h"

#define MAX_NUM_CLIENTS 8

#define MAX_NUM_TSCAN_THREADS             MAX_NUM_CLIENTS * 2 // Q4 has two scans
#define MAX_NUM_AGGREGATE_THREADS         MAX_NUM_CLIENTS
#define MAX_NUM_PARTIAL_AGGREGATE_THREADS MAX_NUM_CLIENTS
#define MAX_NUM_HASH_JOIN_THREADS         MAX_NUM_CLIENTS
#define MAX_NUM_FUNC_CALL_THREADS         MAX_NUM_CLIENTS


void register_stage_containers() {

    register_stage<tscan_stage_t>(MAX_NUM_CLIENTS);
    register_stage<aggregate_stage_t>(MAX_NUM_CLIENTS);
    register_stage<partial_aggregate_stage_t>(MAX_NUM_CLIENTS);
    register_stage<hash_join_stage_t>(MAX_NUM_CLIENTS);
    register_stage<func_call_stage_t>(MAX_NUM_FUNC_CALL_THREADS);

    register_stage<sort_stage_t>(MAX_NUM_CLIENTS);
    register_stage<sorted_in_stage_t>(MAX_NUM_CLIENTS);

    //    register_stage<merge_stage_t>(MAX_NUM_CLIENTS);
    //    register_stage<fdump_stage_t>(MAX_NUM_CLIENTS);
    //    register_stage<fscan_stage_t>(MAX_NUM_CLIENTS);
}
