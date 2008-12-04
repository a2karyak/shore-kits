/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_tpcc_xct.cpp
 *
 *  @brief:  Declaration of the Shore DORA transactions as part of ShoreTPCCEnv
 *
 *  @author: Ippokratis Pandis, Sept 2008
 */


#include "dora/tpcc/dora_payment.h"
#include "dora/tpcc/dora_mbench.h"
#include "dora/tpcc/dora_tpcc.h"


ENTER_NAMESPACE(dora);

using namespace shore;
using namespace tpcc;


typedef range_partition_impl<int>   irpImpl; 


/******** Exported functions  ********/


/********
 ******** Caution: The functions below should be invoked inside
 ********          the context of a smthread
 ********/


/******************************************************************** 
 *
 * TPC-C DORA TRXS
 *
 * (1) The dora_XXX functions are wrappers to the real transactions
 * (2) The xct_dora_XXX functions are the implementation of the transactions
 *
 ********************************************************************/


/******************************************************************** 
 *
 * TPC-C DORA TRXs Wrappers
 *
 * @brief: They are wrappers to the functions that execute the transaction
 *         body. Their responsibility is to:
 *
 *         1. Prepare the corresponding input
 *         2. Check the return of the trx function and abort the trx,
 *            if something went wrong
 *         3. Update the tpcc db environment statistics
 *
 ********************************************************************/


/* --- with input specified --- */

w_rc_t DoraTPCCEnv::dora_new_order(const int xct_id, 
                                   new_order_input_t& anoin,
                                   trx_result_tuple_t& atrt)
{
    TRACE( TRACE_TRX_FLOW, "%d. DORA NEW-ORDER...\n", xct_id);     

    // 1. enqueue all actions
    assert (0);

    return (RCOK); 
}


w_rc_t DoraTPCCEnv::dora_payment(const int xct_id, 
                                 payment_input_t& apin,
                                 trx_result_tuple_t& atrt)
{
    // 1. Initiate transaction
    tid_t atid;   

#ifndef ONLYDORA
    W_DO(_pssm->begin_xct(atid));
#endif

    xct_t* pxct = smthread_t::me()->xct();

#ifndef ONLYDORA
    assert (pxct);
#endif

    TRACE( TRACE_TRX_FLOW, "Begin (%d)\n", atid);

    // 2. Setup the next RVP
    // PH1 consists of 3 packets
    midway_pay_rvp* rvp = NewMidayPayRvp(atid,pxct,xct_id,atrt,apin);
    
    // 3. Generate the actions    
    upd_wh_pay_action* pay_upd_wh     = NewUpdWhPayAction(atid,pxct,rvp,apin);
    upd_dist_pay_action* pay_upd_dist = NewUpdDistPayAction(atid,pxct,rvp,apin);
    upd_cust_pay_action* pay_upd_cust = NewUpdCustPayAction(atid,pxct,rvp,apin);


    // 4. Detatch self from xct

#ifndef ONLYDORA
    me()->detach_xct(pxct);
#endif

    TRACE( TRACE_TRX_FLOW, "Detached from (%d)\n", atid);


    // For each action
    // 5a. Decide about partition
    // 5b. Enqueue
    //
    // All the enqueues should appear atomic
    // That is, there should be a total order across trxs 
    // (it terms of the sequence actions are enqueued)

    {
        int wh = apin._home_wh_id-1;

        // first, figure out to which partitions to enqueue
        irpImpl* my_wh_part   = whs()->myPart(wh);
        irpImpl* my_dist_part = dis()->myPart(wh);
        irpImpl* my_cust_part = cus()->myPart(wh);

        // then, start enqueueing

        // WH_PART_CS
        CRITICAL_SECTION(wh_part_cs, my_wh_part->_enqueue_lock);
            
        if (my_wh_part->enqueue(pay_upd_wh)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing PAY_UPD_WH\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }

        // DIS_PART_CS
        CRITICAL_SECTION(dis_part_cs, my_dist_part->_enqueue_lock);
        wh_part_cs.exit();

        if (my_dist_part->enqueue(pay_upd_dist)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing PAY_UPD_DIST\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }

        // CUS_PART_CS
        CRITICAL_SECTION(cus_part_cs, my_cust_part->_enqueue_lock);
        dis_part_cs.exit();

        if (my_cust_part->enqueue(pay_upd_cust)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing PAY_UPD_CUST\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }
    }

    return (RCOK); 
}


w_rc_t DoraTPCCEnv::dora_order_status(const int xct_id, 
                                      order_status_input_t& aordstin,
                                       trx_result_tuple_t& atrt)
{
    TRACE( TRACE_TRX_FLOW, "%d. ORDER-STATUS...\n", xct_id);

    // 1. enqueue all actions
    assert (0);

    return (RCOK); 
}


w_rc_t DoraTPCCEnv::dora_delivery(const int xct_id, 
                                  delivery_input_t& adelin,
                                  trx_result_tuple_t& atrt)
{
    TRACE( TRACE_TRX_FLOW, "%d. DELIVERY...\n", xct_id);     

    // 1. enqueue all actions
    assert (0);

    return (RCOK); 
}

w_rc_t DoraTPCCEnv::dora_stock_level(const int xct_id, 
                                     stock_level_input_t& astoin,
                                     trx_result_tuple_t& atrt)
{
    TRACE( TRACE_TRX_FLOW, "%d. STOCK-LEVEL...\n", xct_id);     

    // 1. enqueue all actions
    assert (0);

    return (RCOK); 
}



// --- without input specified --- //

w_rc_t DoraTPCCEnv::dora_new_order(const int xct_id, 
                                   trx_result_tuple_t& atrt,
                                   int specificWH)
{
    new_order_input_t noin = create_no_input(_queried_factor, 
                                             specificWH);
    return (dora_new_order(xct_id, noin, atrt));
}


w_rc_t DoraTPCCEnv::dora_payment(const int xct_id, 
                                 trx_result_tuple_t& atrt,
                                 int specificWH)
{
    payment_input_t pin = create_payment_input(_queried_factor, 
                                               specificWH);
    return (dora_payment(xct_id, pin, atrt));
}


w_rc_t DoraTPCCEnv::dora_order_status(const int xct_id, 
                                      trx_result_tuple_t& atrt,
                                      int specificWH)
{
    order_status_input_t ordin = create_order_status_input(_queried_factor, 
                                                           specificWH);
    return (dora_order_status(xct_id, ordin, atrt));
}


w_rc_t DoraTPCCEnv::dora_delivery(const int xct_id, 
                                  trx_result_tuple_t& atrt,
                                  int specificWH)
{
    delivery_input_t delin = create_delivery_input(_queried_factor, 
                                                   specificWH);
    return (dora_delivery(xct_id, delin, atrt));
}


w_rc_t DoraTPCCEnv::dora_stock_level(const int xct_id, 
                                     trx_result_tuple_t& atrt,
                                     int specificWH)
{
    stock_level_input_t slin = create_stock_level_input(_queried_factor, 
                                                        specificWH);
    return (dora_stock_level(xct_id, slin, atrt));
}



/******************************************************************** 
 *
 * DORA MBENCHES
 *
 ********************************************************************/

w_rc_t DoraTPCCEnv::dora_mbench_wh(const int xct_id, 
                                   trx_result_tuple_t& atrt, 
                                   int whid)
{
    // pick a valid wh id
    if (whid==0) 
        whid = URand(1,_scaling_factor); 

    // 1. Initiate transaction
    tid_t atid;   

#ifndef ONLYDORA
    W_DO(_pssm->begin_xct(atid));
    //    W_DO(_pssm->set_lock_cache_enable(false);
#endif

    xct_t* pxct = smthread_t::me()->xct();

#ifndef ONLYDORA
    assert (pxct);
#endif

    TRACE( TRACE_TRX_FLOW, "Begin (%d)\n", atid);

    // 2. Setup the final RVP
    final_mb_rvp* frvp = NewFinalMbRvp(atid,pxct,xct_id,atrt);    

    // 3. Generate the actions
    upd_wh_mb_action* upd_wh = NewUpdWhMbAction(atid,pxct,frvp,whid);

    // 4. Detatch self from xct

#ifndef ONLYDORA
    smthread_t::me()->detach_xct(pxct);
#endif

    TRACE( TRACE_TRX_FLOW, "Detached from (%d)\n", atid);

    // For each action
    // 5a. Decide about partition
    // 5b. Enqueue
    //
    // All the enqueues should appear atomic
    // That is, there should be a total order across trxs 
    // (it terms of the sequence actions are enqueued)

    {        
        irpImpl* mypartition = whs()->myPart(whid-1);

        // WH_PART_CS
        CRITICAL_SECTION(wh_part_cs, mypartition->_enqueue_lock);
        if (mypartition->enqueue(upd_wh)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing UPD_WH_MB\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }
    }

    return (RCOK); 
}

w_rc_t DoraTPCCEnv::dora_mbench_cust(const int xct_id, 
                                     trx_result_tuple_t& atrt, 
                                     int whid)
{
    // pick a valid wh id
    if (whid==0) 
        whid = URand(1,_scaling_factor); 

    // 1. Initiate transaction
    tid_t atid;   

#ifndef ONLYDORA
    W_DO(_pssm->begin_xct(atid));
#endif

    xct_t* pxct = smthread_t::me()->xct();

#ifndef ONLYDORA
    assert (pxct);
#endif

    TRACE( TRACE_TRX_FLOW, "Begin (%d)\n", atid);


    // 2. Setup the final RVP
    final_mb_rvp* frvp = NewFinalMbRvp(atid,pxct,xct_id,atrt);
    
    // 3. Generate the actions
    upd_cust_mb_action* upd_cust = NewUpdCustMbAction(atid,pxct,frvp,whid);

    // 4. Detatch self from xct

#ifndef ONLYDORA
    me()->detach_xct(pxct);
#endif

    TRACE( TRACE_TRX_FLOW, "Detached from (%d)\n", atid);


    // For each action
    // 5a. Decide about partition
    // 5b. Enqueue
    //
    // All the enqueues should appear atomic
    // That is, there should be a total order across trxs 
    // (it terms of the sequence actions are enqueued)

    {
        irpImpl* mypartition = cus()->myPart(whid-1);
        
        // CUS_PART_CS
        CRITICAL_SECTION(cus_part_cs, mypartition->_enqueue_lock);
        if (mypartition->enqueue(upd_cust)) { 
            TRACE( TRACE_DEBUG, "Problem in enqueueing UPD_CUST\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }
    }

    return (RCOK); 
}


EXIT_NAMESPACE(dora);
