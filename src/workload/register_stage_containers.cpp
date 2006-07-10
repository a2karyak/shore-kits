/* -*- mode:C++; c-basic-offset:4 -*- */

#include "trace.h"
#include "tests/common/register_stage.h"

#include "engine/stages/tscan.h"
#include "engine/stages/aggregate.h"
#include "engine/stages/partial_aggregate.h"
#include "engine/stages/hash_join.h"

#define MAX_NUM_CLIENTS 32

#define MAX_NUM_TSCAN_THREADS             MAX_NUM_CLIENTS * 2 // Q4 has two scans
#define MAX_NUM_AGGREGATE_THREADS         MAX_NUM_CLIENTS
#define MAX_NUM_PARTIAL_AGGREGATE_THREADS MAX_NUM_CLIENTS
#define MAX_NUM_HASH_JOIN_THREADS         MAX_NUM_CLIENTS


void register_stage_containers() {

    register_stage<tscan_stage_t>(MAX_NUM_TSCAN_THREADS);
    register_stage<aggregate_stage_t>(MAX_NUM_AGGREGATE_THREADS);
    register_stage<partial_aggregate_stage_t>(MAX_NUM_PARTIAL_AGGREGATE_THREADS);
    register_stage<hash_join_stage_t>(MAX_NUM_HASH_JOIN_THREADS);
}
