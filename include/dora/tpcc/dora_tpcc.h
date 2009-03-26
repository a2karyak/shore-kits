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
#include "dora.h"


ENTER_NAMESPACE(dora);


using namespace shore;
using namespace tpcc;


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
class r_cust_ordst_action;
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
class midway_nord_rvp;
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

class DoraTPCCEnv : public ShoreTPCCEnv
{
public:

    typedef range_partition_impl<int>   irpImpl; 
    typedef range_part_table_impl<int>  irpTableImpl;

    typedef irpImpl::RangeAction  irpAction;

    typedef vector<irpTableImpl*>       irpTablePtrVector;
    typedef irpTablePtrVector::iterator irpTablePtrVectorIt;

private:

    // a vector of pointers to integer-range-partitioned tables
    irpTablePtrVector _irptp_vec;    


    // tpcc setup variables
    int _starting_cpu;
    int _cpu_table_step;
    int _cpu_range;
    int _sf;

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
    virtual const int conf();
    virtual const int info();    
    virtual const int statistics();    




    //// Client API
    
    // enqueues action, false on error
    inline const int enqueue(irpAction* paction, 
                             const bool bWake,
                             irpTableImpl* ptable, 
                             const int part_pos) 
    {
        assert (paction);
        assert (ptable);
        return (ptable->enqueue(paction, bWake, part_pos));
    }



    //// Partition-related methods

    inline irpImpl* table_part(const int table_pos, const int part_pos) {
        assert (table_pos<_irptp_vec.size());        
        return (_irptp_vec[table_pos]->get_part(part_pos));
    }


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



    //// DORA TPCC ACTIONs and RVPs generating functions

    typedef vector<base_action_t*> baseActionsList;
    


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
    //const tpcc_warehouse_tuple& awh
    //const tpcc_district_tuple& adist



    ///////////////////////
    // TPC-C OrderStatus //
    ///////////////////////

    DECLARE_DORA_FINAL_RVP_GEN_FUNC(final_ordst_rvp);

    DECLARE_DORA_ACTION_GEN_FUNC(r_cust_ordst_action,rvp_t,order_status_input_t);
    DECLARE_DORA_ACTION_GEN_FUNC(r_ol_ordst_action,rvp_t,order_status_input_t);
    //const tpcc_order_tuple& aorder



    //////////////////////
    // TPC-C StockLevel //
    //////////////////////

    DECLARE_DORA_MIDWAY_RVP_GEN_FUNC(mid1_stock_rvp,stock_level_input_t);
    DECLARE_DORA_MIDWAY_RVP_WITH_PREV_GEN_FUNC(mid2_stock_rvp,stock_level_input_t);

    DECLARE_DORA_FINAL_RVP_WITH_PREV_GEN_FUNC(final_stock_rvp);


    DECLARE_DORA_ACTION_GEN_FUNC(r_dist_stock_action,mid1_stock_rvp,stock_level_input_t);
    DECLARE_DORA_ACTION_GEN_FUNC(r_ol_stock_action,mid2_stock_rvp,stock_level_input_t);
    //const int nextid

    DECLARE_DORA_ACTION_GEN_FUNC(r_st_stock_action,rvp_t,stock_level_input_t);
    //TwoIntVec* pvwi



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


    DECLARE_DORA_MIDWAY_RVP_GEN_FUNC(midway_nord_rvp,new_order_input_t);
    //final_nord_rvp* frvp

    DECLARE_DORA_FINAL_RVP_GEN_FUNC(final_nord_rvp);
    // const int ol_cnt


    DECLARE_DORA_ACTION_GEN_FUNC(r_wh_nord_action,rvp_t,int);
    //const int d_id

    DECLARE_DORA_ACTION_GEN_FUNC(r_cust_nord_action,rvp_t,int);
    //const int d_id
    //const int c_id


    DECLARE_DORA_ACTION_GEN_FUNC(upd_dist_nord_action,midway_nord_rvp,int);
    //const int d_id

    DECLARE_DORA_ACTION_GEN_FUNC(r_item_nord_action,midway_nord_rvp,int);
    //const int d_id
    //const int olidx

    DECLARE_DORA_ACTION_GEN_FUNC(upd_sto_nord_action,midway_nord_rvp,int);
    //const int d_id
    //const int olidx

    DECLARE_DORA_ACTION_GEN_FUNC(ins_ord_nord_action,rvp_t,int);
    //const int d_id
    //const int nextoid
    //const cid
    //const tstamp
    //const int olcnt
    //const int alllocal

    DECLARE_DORA_ACTION_GEN_FUNC(ins_nord_nord_action,rvp_t,int);
    //const int d_id
    //const int nextoid

    DECLARE_DORA_ACTION_GEN_FUNC(ins_ol_nord_action,rvp_t,int);
    //const int d_id
    //const int nextoid
    //const int olidx
    //const ol_item_info& iteminfo
    //time_t tstamp


private:

    // algorithm for deciding the distribution of tables 
    const processorid_t _next_cpu(const processorid_t& aprd,
                                  const irpTableImpl* atable,
                                  const int step=DF_CPU_STEP_TABLES);



        
}; // EOF: DoraTPCCEnv



EXIT_NAMESPACE(dora);

#endif /** __DORA_TPCC_H */

