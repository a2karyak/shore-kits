/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file register_stage_containers.h
 *
 *  @brief Registration of the stages on the system, given the environment
 *
 *  @history
 *  10/21/07: Adding options for InMemory and SHORE transactional environments
 */

#include "workload/register_stage_containers.h"

#include "stages.h"
#include "trxstages.h"

#include "workload/common/register_stage.h"


// OLTP-related constants
#define MAX_NUM_TRXS     64


// DSS-related constants
//#define MAX_NUM_CLIENTS 16
#define MAX_NUM_CLIENTS 256
#define MAX_NUM_TSCAN_THREADS             MAX_NUM_CLIENTS * 2 // Q4, Q16 have two scans
#define MAX_NUM_AGGREGATE_THREADS         MAX_NUM_CLIENTS
#define MAX_NUM_PARTIAL_AGGREGATE_THREADS MAX_NUM_CLIENTS * 2 // Q1 uses two partial aggregates
#define MAX_NUM_HASH_JOIN_THREADS         MAX_NUM_CLIENTS
#define MAX_NUM_FUNC_CALL_THREADS         MAX_NUM_CLIENTS
#define MAX_NUM_SORT_THREADS              MAX_NUM_CLIENTS * 2 // Q16 uses two sorts
#define MAX_NUM_SORTED_IN_STAGE_THREADS   MAX_NUM_CLIENTS


void register_stage_containers(int environment) {


    if (environment != QUERY_ENV) {
        TRACE( TRACE_ALWAYS, "\n*****************\n" \
               "Registering TRX environment\n" \
               "SCALING FACTOR \t\t= (%d)\nQUERIED WAREHOUSES \t= (%d)\n" \
               "*****************\n",
               TPCC_SCALING_FACTOR, QUERIED_TPCC_SCALING_FACTOR);
    }
    
    switch (environment) {

        /** OLTP stages registration */
        /** @note trx stages do not share */

    case TRX_BDB_ENV:

        /*
        register_stage<payment_begin_stage_t>(MAX_NUM_CLIENTS, false); 
        register_stage<payment_upd_wh_stage_t>(MAX_NUM_CLIENTS, false); 
        register_stage<payment_upd_distr_stage_t>(MAX_NUM_CLIENTS, false); 
        register_stage<payment_upd_cust_stage_t>(MAX_NUM_CLIENTS, false); 
        register_stage<payment_ins_hist_stage_t>(MAX_NUM_CLIENTS, false); 
        register_stage<payment_finalize_stage_t>(MAX_NUM_CLIENTS, false); 
        */

        register_stage<payment_baseline_stage_t>(MAX_NUM_TRXS, false);      
        break;

    case TRX_SHORE_ENV:

        assert (1==0); // (ip) Not implemented yet       
        break;

    case TRX_MEM_ENV:

        register_stage<inmem_payment_baseline_stage_t>(MAX_NUM_TRXS, false);
        break;

        /** DSS stages registration */
    case QUERY_ENV:
    default:

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
        break;
    }
}

