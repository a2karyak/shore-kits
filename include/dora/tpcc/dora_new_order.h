/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_new_order.h
 *
 *  @brief:  DORA TPC-C NEW_ORDER
 *
 *  @note:   Definition of RVPs and Actions that synthesize 
 *           the TPC-C NewOrder trx according to DORA
 *
 *  @author: Ippokratis Pandis, Nov 2008
 */


#ifndef __DORA_TPCC_NEW_ORDER_H
#define __DORA_TPCC_NEW_ORDER_H


#include "dora/tpcc/dora_tpcc.h"
#include "workload/tpcc/shore_tpcc_env.h"

using namespace shore;
using namespace tpcc;


ENTER_NAMESPACE(dora);




//
// RVPS
//
// (1) final_nord_rvp
// (2) midway_nord_rvp
//


DECLARE_DORA_EMPTY_MIDWAY_RVP_CLASS(mid_nord_rvp,DoraTPCCEnv,new_order_input_t,5,5);

DECLARE_DORA_FINAL_RVP_CLASS(final_nord_rvp,DoraTPCCEnv,3,8);




//
// ACTIONS
//

//
// Start -> Midway
//
// (1) r_wh_nord_action
// (2) r_cust_nord_action
// (3) upd_dist_nord_action
// (4) r_item_nord_action
// (5) upd_sto_nord_action
// 


DECLARE_DORA_ACTION_WITH_RVP_CLASS(r_wh_nord_action,int,DoraTPCCEnv,mid_nord_rvp,no_item_nord_input_t,1);

DECLARE_DORA_ACTION_WITH_RVP_CLASS(upd_dist_nord_action,int,DoraTPCCEnv,mid_nord_rvp,no_item_nord_input_t,2);

// !!! 2 fields only (WH,DI) determine the CUSTOMER table accesses, not 3 !!!
DECLARE_DORA_ACTION_WITH_RVP_CLASS(r_cust_nord_action,int,DoraTPCCEnv,mid_nord_rvp,no_item_nord_input_t,2);

// !!! The items reading is performed as a single action (instead of OLCNT)
// !!! 1 field only (WH) determines the ITEM table accesses, not 2 and not ITEM !!!
DECLARE_DORA_ACTION_WITH_RVP_CLASS(r_item_nord_action,int,DoraTPCCEnv,mid_nord_rvp,new_order_input_t,1);

// !!! The stocks updating is performed as a single action (instead of OLCNT)
// !!! 1 field only (WH) determines the STOCK table accesses, not 2 !!!
DECLARE_DORA_ACTION_WITH_RVP_CLASS(upd_sto_nord_action,int,DoraTPCCEnv,mid_nord_rvp,new_order_input_t,1);



//
// Midway -> Final
//
// (6) ins_ord_nord_action
// (7) ins_nord_nord_action
// (8) ins_ol_nord_action
//

// !!! 2 fields only (WH,DI) determine the ORDER table accesses, not 3 !!!
DECLARE_DORA_ACTION_NO_RVP_CLASS(ins_ord_nord_action,int,DoraTPCCEnv,no_item_nord_input_t,2);

// !!! 2 fields only (WH,DI) determine the NEW-ORDER table accesses, not 3 !!!
DECLARE_DORA_ACTION_NO_RVP_CLASS(ins_nord_nord_action,int,DoraTPCCEnv,no_item_nord_input_t,2);


// !!! 2 fields only (WH,DI) determine the ORDERLINE table accesses, not 3 !!!
DECLARE_DORA_ACTION_NO_RVP_CLASS(ins_ol_nord_action,int,DoraTPCCEnv,new_order_input_t,2);




EXIT_NAMESPACE(dora);

#endif /** __DORA_TPCC_NEW_ORDER_H */

