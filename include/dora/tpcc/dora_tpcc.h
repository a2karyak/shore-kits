/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_tpcc.h
 *
 *  @brief:  The DORA TPC-C class
 *
 *  @author: Ippokratis Pandis, Oct 2008
 */


#ifndef __DORA_TPCC_H
#define __DORA_TPCC_H


#include <cstdio>

#include "tls.h"

#include "util.h"
#include "workload/tpcc/shore_tpcc_env.h"
#include "dora/dora_env.h"
#include "dora.h"

using namespace shore;
using namespace tpcc;


ENTER_NAMESPACE(dora);



// Forward declarations

// MBenches
class final_mb_rvp;
class upd_wh_mb_action;
class upd_cust_mb_action;

// TPC-C Payment
class final_pay_rvp;
class midway_pay_rvp;
class upd_wh_pay_action;
class upd_dist_pay_action;
class upd_cust_pay_action;
class ins_hist_pay_action;

// TPC-C OrderStatus
class final_ordst_rvp;
class mid1_ordst_rvp;
class mid2_ordst_rvp;
class r_cust_ordst_action;
class r_ord_ordst_action;
class r_ol_ordst_action;


// TPC-C StockLevel
class mid1_stock_rvp;
class mid2_stock_rvp;
class final_stock_rvp;
class r_dist_stock_action;
class r_ol_stock_action;
class r_st_stock_action;

// TPC-C Delivery
class mid1_del_rvp;
class mid2_del_rvp;
class final_del_rvp;
class del_nord_del_action;
class upd_ord_del_action;
class upd_oline_del_action;
class upd_cust_del_action;

// TPC-C NewOrder
class mid_nord_rvp;
class final_nord_rvp;
class r_wh_nord_action;
class r_cust_nord_action;
class upd_dist_nord_action;
class r_item_nord_action;
class upd_sto_nord_action;
class ins_ord_nord_action;
class ins_nord_nord_action;
class ins_ol_nord_action;



/******************************************************************** 
 *
 * @class: dora_tpcc
 *
 * @brief: Container class for all the data partitions for the TPC-C database
 *
 ********************************************************************/

class DoraTPCCEnv : public ShoreTPCCEnv, public DoraEnv
{
public:
    
    DoraTPCCEnv(string confname)
        : ShoreTPCCEnv(confname)
    { }

    virtual ~DoraTPCCEnv() 
    { 
        stop();
    }



    //// Control Database

    // {Start/Stop/Resume/Pause} the system 
    const int start();
    const int stop();
    const int resume();
    const int pause();
    const int newrun();
    const int set(envVarMap* vars);
    const int dump();
    const int info();    
    const int statistics();    
    const int conf();


    //// DORA TPCC TABLE PARTITIONS
    DECLARE_DORA_PARTS(whs);
    DECLARE_DORA_PARTS(dis);
    DECLARE_DORA_PARTS(cus);
    DECLARE_DORA_PARTS(his);
    DECLARE_DORA_PARTS(nor);
    DECLARE_DORA_PARTS(ord);
    DECLARE_DORA_PARTS(ite);
    DECLARE_DORA_PARTS(oli);
    DECLARE_DORA_PARTS(sto);


    //// DORA TPCC TRXs

    DECLARE_DORA_TRX(new_order);
    DECLARE_DORA_TRX(payment);
    DECLARE_DORA_TRX(order_status);
    DECLARE_DORA_TRX(delivery);
    DECLARE_DORA_TRX(stock_level);

    DECLARE_DORA_TRX(mbench_wh);
    DECLARE_DORA_TRX(mbench_cust);
   


    //////////////
    // MBenches //
    //////////////

    DECLARE_DORA_FINAL_RVP_GEN_FUNC(final_mb_rvp);


    DECLARE_DORA_ACTION_GEN_FUNC(upd_wh_mb_action,rvp_t,mbench_wh_input_t);

    DECLARE_DORA_ACTION_GEN_FUNC(upd_cust_mb_action,rvp_t,mbench_cust_input_t);

    

    ///////////////////
    // TPC-C Payment //
    ///////////////////

    DECLARE_DORA_MIDWAY_RVP_GEN_FUNC(midway_pay_rvp,payment_input_t);

    DECLARE_DORA_FINAL_RVP_WITH_PREV_GEN_FUNC(final_pay_rvp);


    DECLARE_DORA_ACTION_GEN_FUNC(upd_wh_pay_action,midway_pay_rvp,payment_input_t);
    DECLARE_DORA_ACTION_GEN_FUNC(upd_dist_pay_action,midway_pay_rvp,payment_input_t);
    DECLARE_DORA_ACTION_GEN_FUNC(upd_cust_pay_action,midway_pay_rvp,payment_input_t);

    DECLARE_DORA_ACTION_GEN_FUNC(ins_hist_pay_action,rvp_t,payment_input_t);



    ///////////////////////
    // TPC-C OrderStatus //
    ///////////////////////

    DECLARE_DORA_MIDWAY_RVP_GEN_FUNC(mid1_ordst_rvp,order_status_input_t);

    DECLARE_DORA_MIDWAY_RVP_WITH_PREV_GEN_FUNC(mid2_ordst_rvp,order_status_input_t);

    DECLARE_DORA_FINAL_RVP_WITH_PREV_GEN_FUNC(final_ordst_rvp);

    DECLARE_DORA_ACTION_GEN_FUNC(r_cust_ordst_action,mid1_ordst_rvp,order_status_input_t);
    DECLARE_DORA_ACTION_GEN_FUNC(r_ord_ordst_action,mid2_ordst_rvp,order_status_input_t);

    DECLARE_DORA_ACTION_GEN_FUNC(r_ol_ordst_action,rvp_t,order_status_input_t);


    //////////////////////
    // TPC-C StockLevel //
    //////////////////////

    DECLARE_DORA_MIDWAY_RVP_GEN_FUNC(mid1_stock_rvp,stock_level_input_t);
    DECLARE_DORA_MIDWAY_RVP_WITH_PREV_GEN_FUNC(mid2_stock_rvp,stock_level_input_t);

    DECLARE_DORA_FINAL_RVP_WITH_PREV_GEN_FUNC(final_stock_rvp);

    DECLARE_DORA_ACTION_GEN_FUNC(r_dist_stock_action,mid1_stock_rvp,stock_level_input_t);
    DECLARE_DORA_ACTION_GEN_FUNC(r_ol_stock_action,mid2_stock_rvp,stock_level_input_t);

    DECLARE_DORA_ACTION_GEN_FUNC(r_st_stock_action,rvp_t,stock_level_input_t);



    ////////////////////
    // TPC-C Delivery //
    ////////////////////


    DECLARE_DORA_MIDWAY_RVP_GEN_FUNC(mid1_del_rvp,delivery_input_t);
    //final_del_rvp* frvp
    //const int d_id

    DECLARE_DORA_MIDWAY_RVP_WITH_PREV_GEN_FUNC(mid2_del_rvp,delivery_input_t);
    //final_del_rvp* frvp
    //const int d_id


    DECLARE_DORA_FINAL_RVP_GEN_FUNC(final_del_rvp);


    DECLARE_DORA_ACTION_GEN_FUNC(del_nord_del_action,mid1_del_rvp,delivery_input_t);
    //const int d_id

    DECLARE_DORA_ACTION_GEN_FUNC(upd_ord_del_action,mid2_del_rvp,delivery_input_t);
    //const int d_id
    //const int o_id

    DECLARE_DORA_ACTION_GEN_FUNC(upd_oline_del_action,mid2_del_rvp,delivery_input_t);
    //const int d_id
    //const int o_id

    DECLARE_DORA_ACTION_GEN_FUNC(upd_cust_del_action,rvp_t,delivery_input_t);
    //const int d_id
    //const int o_id
    //const int amount




    ////////////////////
    // TPC-C NewOrder //
    ////////////////////


    DECLARE_DORA_MIDWAY_RVP_GEN_FUNC(mid_nord_rvp,new_order_input_t);

    DECLARE_DORA_FINAL_RVP_WITH_PREV_GEN_FUNC(final_nord_rvp);


    // Start -> Midway
    DECLARE_DORA_ACTION_GEN_FUNC(r_wh_nord_action,mid_nord_rvp,no_item_nord_input_t);

    DECLARE_DORA_ACTION_GEN_FUNC(upd_dist_nord_action,mid_nord_rvp,no_item_nord_input_t);

    DECLARE_DORA_ACTION_GEN_FUNC(r_cust_nord_action,mid_nord_rvp,no_item_nord_input_t);

    DECLARE_DORA_ACTION_GEN_FUNC(r_item_nord_action,mid_nord_rvp,new_order_input_t);

    DECLARE_DORA_ACTION_GEN_FUNC(upd_sto_nord_action,mid_nord_rvp,new_order_input_t);


    // Midway -> Final
    DECLARE_DORA_ACTION_GEN_FUNC(ins_ord_nord_action,rvp_t,no_item_nord_input_t);

    DECLARE_DORA_ACTION_GEN_FUNC(ins_nord_nord_action,rvp_t,no_item_nord_input_t);

    DECLARE_DORA_ACTION_GEN_FUNC(ins_ol_nord_action,rvp_t,new_order_input_t);

        
}; // EOF: DoraTPCCEnv



EXIT_NAMESPACE(dora);

#endif /** __DORA_TPCC_H */

