/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file register_stage_containers.h
 *
 *  @brief Registration of the stages on the system, given the environment
 *
 *  @history
 *  10/21/07: Adding options for InMemory and SHORE transactional environments
 */

#include "workload/register_stage_containers.h"
#include "workload/register_stage.h"

#include "trxstages.h"



// OLTP-related constants
const int MAX_NUM_TRXS = 32;


// DSS-related constants
//#define MAX_NUM_CLIENTS 16
const int MAX_NUM_CLIENTS = 256;


void register_stage_containers(int environment) 
{
    switch (environment) {

        /** OLTP stages registration */
        /** @note trx stages do not share */

    case TRX_SHORE_ENV:
    default:

//         // Baseline transactions
//         register_stage<shore_new_order_baseline_stage_t>(MAX_NUM_TRXS, false);
//         register_stage<shore_payment_baseline_stage_t>(MAX_NUM_TRXS, false);
//         register_stage<shore_order_status_baseline_stage_t>(MAX_NUM_TRXS, false);
//         register_stage<shore_delivery_baseline_stage_t>(MAX_NUM_TRXS, false);
//         register_stage<shore_stock_level_baseline_stage_t>(MAX_NUM_TRXS, false);


//         // Staged transactions

//         // for Payment
//         register_stage<shore_payment_begin_stage_t>(MAX_NUM_TRXS, false); 
//         register_stage<shore_payment_upd_wh_stage_t>(MAX_NUM_TRXS, false); 
//         register_stage<shore_payment_upd_distr_stage_t>(MAX_NUM_TRXS, false); 
//         register_stage<shore_payment_upd_cust_stage_t>(MAX_NUM_TRXS, false); 
//         register_stage<shore_payment_ins_hist_stage_t>(MAX_NUM_TRXS, false); 
//         register_stage<shore_payment_finalize_stage_t>(MAX_NUM_TRXS, false); 

//         // for NewOrder
//         register_stage<shore_new_order_begin_stage_t>(MAX_NUM_TRXS, false); 
//         register_stage<shore_new_order_outside_loop_stage_t>(MAX_NUM_TRXS, false); 
//         register_stage<shore_new_order_one_ol_stage_t>(MAX_NUM_TRXS, false); 
//         register_stage<shore_new_order_finalize_stage_t>(MAX_NUM_TRXS, false); 


        break;
    }
}

