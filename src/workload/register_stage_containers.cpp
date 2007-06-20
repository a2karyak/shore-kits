/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages.h"
#include "workload/common/register_stage.h"

//#define MAX_NUM_CLIENTS 16
#define MAX_NUM_CLIENTS 128

#define MAX_NUM_TSCAN_THREADS             MAX_NUM_CLIENTS * 2 // Q4, Q16 have two scans
#define MAX_NUM_AGGREGATE_THREADS         MAX_NUM_CLIENTS
#define MAX_NUM_PARTIAL_AGGREGATE_THREADS MAX_NUM_CLIENTS * 2 // Q1 uses two partial aggregates
#define MAX_NUM_HASH_JOIN_THREADS         MAX_NUM_CLIENTS
#define MAX_NUM_FUNC_CALL_THREADS         MAX_NUM_CLIENTS
#define MAX_NUM_SORT_THREADS              MAX_NUM_CLIENTS * 2 // Q16 uses two sorts
#define MAX_NUM_SORTED_IN_STAGE_THREADS   MAX_NUM_CLIENTS


void register_stage_containers() {

    register_stage<tscan_stage_t>(MAX_NUM_TSCAN_THREADS, true);
    register_stage<aggregate_stage_t>(MAX_NUM_AGGREGATE_THREADS, true);
    register_stage<partial_aggregate_stage_t>(MAX_NUM_PARTIAL_AGGREGATE_THREADS, true);
    register_stage<hash_aggregate_stage_t>(MAX_NUM_AGGREGATE_THREADS, true);
    register_stage<hash_join_stage_t>(MAX_NUM_HASH_JOIN_THREADS, true);
    register_stage<pipe_hash_join_stage_t>(MAX_NUM_CLIENTS, true);
    register_stage<func_call_stage_t>(MAX_NUM_FUNC_CALL_THREADS, true);
    register_stage<sort_stage_t>(MAX_NUM_SORT_THREADS, true);

    register_stage<sorted_in_stage_t>(MAX_NUM_SORTED_IN_STAGE_THREADS, true);
    register_stage<echo_stage_t>(MAX_NUM_CLIENTS, true);
    register_stage<sieve_stage_t>(MAX_NUM_CLIENTS, true);

#if 0
    register_stage<tscan_stage_t>(MAX_NUM_CLIENTS);
    register_stage<aggregate_stage_t>(MAX_NUM_CLIENTS);
    register_stage<partial_aggregate_stage_t>(MAX_NUM_CLIENTS);
    register_stage<hash_aggregate_stage_t>(MAX_NUM_CLIENTS);
    register_stage<hash_join_stage_t>(MAX_NUM_CLIENTS);
    register_stage<pipe_hash_join_stage_t>(MAX_NUM_CLIENTS);
    register_stage<func_call_stage_t>(MAX_NUM_FUNC_CALL_THREADS);
    register_stage<sort_stage_t>(MAX_NUM_SORT_THREADS);

    register_stage<sorted_in_stage_t>(MAX_NUM_CLIENTS);
    register_stage<echo_stage_t>(MAX_NUM_CLIENTS);
    register_stage<sieve_stage_t>(MAX_NUM_CLIENTS);

    //    register_stage<merge_stage_t>(MAX_NUM_CLIENTS);
    //    register_stage<fdump_stage_t>(MAX_NUM_CLIENTS);
    //    register_stage<fscan_stage_t>(MAX_NUM_CLIENTS);
#endif
}
