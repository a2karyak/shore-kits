/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages.h"
#include "workload/common/register_stage.h"
#include "workload/register_stage_containers.h"

#define MAX_NUM_TRXS     1000
//#define MAX_NUM_CLIENTS 16
#define MAX_NUM_CLIENTS 128
#define MAX_NUM_TSCAN_THREADS             MAX_NUM_CLIENTS * 2 // Q4, Q16 have two scans
#define MAX_NUM_AGGREGATE_THREADS         MAX_NUM_CLIENTS
#define MAX_NUM_PARTIAL_AGGREGATE_THREADS MAX_NUM_CLIENTS * 2 // Q1 uses two partial aggregates
#define MAX_NUM_HASH_JOIN_THREADS         MAX_NUM_CLIENTS
#define MAX_NUM_FUNC_CALL_THREADS         MAX_NUM_CLIENTS
#define MAX_NUM_SORT_THREADS              MAX_NUM_CLIENTS * 2 // Q16 uses two sorts
#define MAX_NUM_SORTED_IN_STAGE_THREADS   MAX_NUM_CLIENTS


void register_stage_containers(int enviroment) {



    if (enviroment == QUERY_ENV) {
        register_stage<tscan_stage_t>(MAX_NUM_TSCAN_THREADS, false);
        register_stage<aggregate_stage_t>(MAX_NUM_AGGREGATE_THREADS, false);
        register_stage<partial_aggregate_stage_t>(MAX_NUM_PARTIAL_AGGREGATE_THREADS, false);
        register_stage<hash_aggregate_stage_t>(MAX_NUM_AGGREGATE_THREADS, false);
        register_stage<hash_join_stage_t>(MAX_NUM_HASH_JOIN_THREADS, false);
        register_stage<pipe_hash_join_stage_t>(MAX_NUM_CLIENTS, false);
        register_stage<func_call_stage_t>(MAX_NUM_FUNC_CALL_THREADS, false);
        register_stage<sort_stage_t>(MAX_NUM_SORT_THREADS, true);

        register_stage<sorted_in_stage_t>(MAX_NUM_SORTED_IN_STAGE_THREADS, false);
        register_stage<echo_stage_t>(MAX_NUM_CLIENTS, false);
        register_stage<sieve_stage_t>(MAX_NUM_CLIENTS, false);
    }
    else {        
        // FIXME: (ip) Should be relative with the MAX_NUM_TRXS
        // trx staged do not share
        register_stage<payment_begin_stage_t>(MAX_NUM_CLIENTS, false); 
        register_stage<payment_upd_wh_stage_t>(MAX_NUM_CLIENTS, false); 
        register_stage<payment_upd_distr_stage_t>(MAX_NUM_CLIENTS, false); 
        register_stage<payment_upd_cust_stage_t>(MAX_NUM_CLIENTS, false); 
        register_stage<payment_ins_hist_stage_t>(MAX_NUM_CLIENTS, false); 
        register_stage<payment_finalize_stage_t>(MAX_NUM_CLIENTS, false); 
    }
}

