/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_tpcc_xct.cpp
 *
 *  @brief:  Declaration of the Shore DORA transactions as part of ShoreTPCCEnv
 *
 *  @author: Ippokratis Pandis, Sept 2008
 */

#include "stages/tpcc/shore/shore_tpcc_env.h"

#include "dora/tpcc/dora_payment.h"
#include "dora/tpcc/dora_mbench.h"
#include "dora/tpcc/dora_tpcc.h"


using namespace shore;
using namespace dora;
using namespace tpcc;


ENTER_NAMESPACE(tpcc);


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

w_rc_t ShoreTPCCEnv::dora_new_order(const int xct_id, 
                                    new_order_input_t& anoin,
                                    trx_result_tuple_t& atrt)
{
    TRACE( TRACE_TRX_FLOW, "%d. DORA NEW-ORDER...\n", xct_id);     

    return (RCOK); 
}


w_rc_t ShoreTPCCEnv::dora_payment(const int xct_id, 
                                  payment_input_t& apin,
                                  trx_result_tuple_t& atrt)
{
    assert (_g_dora);

    // 1. Initiate transaction
    tid_t atid;   
    W_DO(_pssm->begin_xct(atid));
    xct_t* pxct = ss_m::tid_to_xct(atid);    
    assert (pxct);
    TRACE( TRACE_TRX_FLOW, "Begin (%d)\n", atid);

    // 2. Setup the next RVP
    midway_pay_rvp* rvp = new midway_pay_rvp(atid, pxct,
                                             atrt, this, apin); // PH1 consists of 3 packets
    assert (rvp);
    
    // 3. generate and enqueue all actions
    //
    // For each generated action. Decide about partition and enqueue it.
    //
    
    // 3a. Borrow the corresponding action
    // 3b. Set rvp, tid, xct, and pin
    // 3c. Decide about partition
    // 3d. Enqueue it

    // @note: Enqueuing in order of trx size 
    // @note: Getting a mutex because all the trxs need to enqueue their actions
    //        in some total order.

    { // DETACH_LOCK_CS

        // The worker threads may finish up before the client detaches.
        // We want the worker threads start detaching after the client does.
        CRITICAL_SECTION(detatch_lock_cs, rvp->_detach_lock);

        upd_cust_pay_action_impl* p_upd_cust_pay_action = 
            _g_dora->get_upd_cust_pay_action();
        assert (p_upd_cust_pay_action);
        p_upd_cust_pay_action->set_input(atid, pxct, rvp, this, apin);
        rvp->set_puc(p_upd_cust_pay_action);
        p_upd_cust_pay_action->_m_rvp=rvp;

        upd_dist_pay_action_impl* p_upd_dist_pay_action = 
            _g_dora->get_upd_dist_pay_action();
        assert (p_upd_dist_pay_action);
        p_upd_dist_pay_action->set_input(atid, pxct, rvp, this, apin);
        rvp->set_pud(p_upd_dist_pay_action);
        p_upd_dist_pay_action->_m_rvp=rvp;

        upd_wh_pay_action_impl*   p_upd_wh_pay_action   = 
            _g_dora->get_upd_wh_pay_action();
        assert (p_upd_wh_pay_action);
        p_upd_wh_pay_action->set_input(atid, pxct, rvp, this, apin);
        rvp->set_puw(p_upd_wh_pay_action);
        p_upd_wh_pay_action->_m_rvp=rvp;

        // Start enqueueing 
        //
        // All the enqueues should appear atomic
        // That is, there should be a total order across trxs 
        // (it terms of the sequence actions are enqueued)

        {
            //CRITICAL_SECTION(pay_enqueue_cs, _g_dora->_pay_lock);
            int mypartition = apin._home_wh_id-1;

            // WH_PART_CS
            CRITICAL_SECTION(wh_part_cs, _g_dora->whs(mypartition)->_enqueue_lock);
            
            // (SF) WAREHOUSE partitions
            if (_g_dora->whs()->enqueue(p_upd_wh_pay_action, mypartition)) {
                TRACE( TRACE_DEBUG, "Problem in enqueueing UPD_WH_PAY\n");
                assert (0); 
                return (RC(de_PROBLEM_ENQUEUE));
            }

            // DIS_PART_CS
            CRITICAL_SECTION(dis_part_cs, _g_dora->dis(mypartition)->_enqueue_lock);
            wh_part_cs.exit();

            // (SF) WAREHOUSE partitions
            if (_g_dora->dis()->enqueue(p_upd_dist_pay_action, mypartition)) {
                TRACE( TRACE_DEBUG, "Problem in enqueueing UPD_DIST_PAY\n");
                assert (0); 
                return (RC(de_PROBLEM_ENQUEUE));
            }

            // CUS_PART_CS
            CRITICAL_SECTION(cus_part_cs, _g_dora->cus(mypartition)->_enqueue_lock);
            dis_part_cs.exit();

            // (SF) WAREHOUSE partitions
            if (_g_dora->cus()->enqueue(p_upd_cust_pay_action, mypartition)) {
                TRACE( TRACE_DEBUG, "Problem in enqueueing UPD_CUST_PAY\n");
                assert (0); 
                return (RC(de_PROBLEM_ENQUEUE));
            }
        }

        // 4. detatch self from xct
        me()->detach_xct(pxct);

        TRACE( TRACE_TRX_FLOW, "Deattached from (%d)\n", atid);

    } // DETACH_LOCK_CS    

    return (RCOK); 
}


w_rc_t ShoreTPCCEnv::dora_order_status(const int xct_id, 
                                       order_status_input_t& aordstin,
                                       trx_result_tuple_t& atrt)
{
    assert (_g_dora);
    TRACE( TRACE_TRX_FLOW, "%d. DORA - EMPTY...\n", xct_id);     

    tid_t atid;   

    // 1. Initiate transaction
    W_DO(_pssm->begin_xct(atid));
    xct_t* pxct = ss_m::tid_to_xct(atid);    
    assert (pxct);

    payment_input_t apin;

    // 2. Setup the next RVP
    final_pay_rvp* rvp = new final_pay_rvp(atid, pxct,
                                           atrt, this);
    assert (rvp);

    ins_hist_pay_action_impl*   p_ins_hist_pay_action   = _g_dora->get_ins_hist_pay_action();
    assert (p_ins_hist_pay_action);
    p_ins_hist_pay_action->set_input(atid, pxct, rvp, this, apin);
    rvp->set_pih(p_ins_hist_pay_action);
    tpcc_warehouse_tuple awh;
    tpcc_district_tuple adist;
    p_ins_hist_pay_action->_awh = awh;
    p_ins_hist_pay_action->_adist = adist;
    if (_g_dora->his()->enqueue(p_ins_hist_pay_action, 0)) { // One HISTORY partition
            TRACE( TRACE_DEBUG, "Problem in enqueueing INS_HIST_PAY\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
    }

    // 4. detatch self from xct
    me()->detach_xct(pxct);

    return (RCOK); 
}


w_rc_t ShoreTPCCEnv::dora_delivery(const int xct_id, 
                                   delivery_input_t& adelin,
                                   trx_result_tuple_t& atrt)
{
    TRACE( TRACE_TRX_FLOW, "%d. DELIVERY...\n", xct_id);     

    // 1. enqueue all actions
    assert (0);

    return (RCOK); 
}

w_rc_t ShoreTPCCEnv::dora_stock_level(const int xct_id, 
                                      stock_level_input_t& astoin,
                                      trx_result_tuple_t& atrt)
{
    TRACE( TRACE_TRX_FLOW, "%d. STOCK-LEVEL...\n", xct_id);     

    // 1. enqueue all actions
    assert (0);

    return (RCOK); 
}



/* --- without input specified --- */

w_rc_t ShoreTPCCEnv::dora_new_order(const int xct_id, 
                                    trx_result_tuple_t& atrt,
                                    int specificWH)
{
    new_order_input_t noin = create_no_input(_queried_factor, 
                                             specificWH);
    return (dora_new_order(xct_id, noin, atrt));
}


w_rc_t ShoreTPCCEnv::dora_payment(const int xct_id, 
                                  trx_result_tuple_t& atrt,
                                  int specificWH)
{
    payment_input_t pin = create_payment_input(_queried_factor, 
                                               specificWH);
    return (dora_payment(xct_id, pin, atrt));
}


w_rc_t ShoreTPCCEnv::dora_order_status(const int xct_id, 
                                       trx_result_tuple_t& atrt,
                                       int specificWH)
{
    order_status_input_t ordin = create_order_status_input(_queried_factor, 
                                                           specificWH);
    return (dora_order_status(xct_id, ordin, atrt));
}


w_rc_t ShoreTPCCEnv::dora_delivery(const int xct_id, 
                                   trx_result_tuple_t& atrt,
                                   int specificWH)
{
    delivery_input_t delin = create_delivery_input(_queried_factor, 
                                                   specificWH);
    return (dora_delivery(xct_id, delin, atrt));
}


w_rc_t ShoreTPCCEnv::dora_stock_level(const int xct_id, 
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

w_rc_t ShoreTPCCEnv::dora_mbench_cust(const int xct_id, trx_result_tuple_t& atrt, const int whid)
{
    assert (_g_dora);

    // 1. Initiate transaction
    tid_t atid;   
    W_DO(_pssm->begin_xct(atid));
    xct_t* pxct = ss_m::tid_to_xct(atid);    
    assert (pxct);
    TRACE( TRACE_TRX_FLOW, "Begin (%d)\n", atid);

    // 2. Setup the final RVP
    final_mb_cust_rvp* frvp = new final_mb_cust_rvp(atid, pxct, atrt, this);
    assert (frvp);
    
    // 3. generate and enqueue action
    // For each generated action. Decide about partition and enqueue it.
    // 3a. Borrow the corresponding action
    // 3b. Set rvp, tid, xct, and pin
    // 3c. Decide about partition
    // 3d. Enqueue it

    { // DETACH_LOCK_CS

        // The worker threads may finish up before the client detaches.
        // We want the worker threads start detaching after the client does.
        CRITICAL_SECTION(detatch_lock_cs, frvp->_detach_lock);
        upd_cust_mb_action_impl*   p_upd_cust_mb_action   = 
            _g_dora->get_upd_cust_mb_action();
        assert (p_upd_cust_mb_action);
        p_upd_cust_mb_action->set_input(atid, pxct, frvp, this, whid);
        frvp->_puc = p_upd_cust_mb_action;

        // Start enqueueing 
        //
        // All the enqueues should appear atomic
        // That is, there should be a total order across trxs 
        // (it terms of the sequence actions are enqueued)

        {
            int mypartition = whid-1;
            // CUS_PART_CS
            CRITICAL_SECTION(cus_part_cs, _g_dora->cus(mypartition)->_enqueue_lock);
            // (SF) CUSTOMER partitions
            if (_g_dora->cus()->enqueue(p_upd_cust_mb_action, mypartition)) { 
                TRACE( TRACE_DEBUG, "Problem in enqueueing UPD_CUST_PAY\n");
                assert (0); 
                return (RC(de_PROBLEM_ENQUEUE));
            }
            cus_part_cs.exit();            
        }

        // 4. detatch self from xct
        me()->detach_xct(pxct);
        TRACE( TRACE_TRX_FLOW, "Deattached from (%d)\n", atid);

    } // DETACH_LOCK_CS    
    return (RCOK); 
}

w_rc_t ShoreTPCCEnv::dora_mbench_wh(const int xct_id, trx_result_tuple_t& atrt, const int whid)
{
    assert (_g_dora);

    // 1. Initiate transaction
    tid_t atid;   
    W_DO(_pssm->begin_xct(atid));
    xct_t* pxct = ss_m::tid_to_xct(atid);    
    assert (pxct);
    TRACE( TRACE_TRX_FLOW, "Begin (%d)\n", atid);

    // 2. Setup the final RVP
    final_mb_wh_rvp* frvp = new final_mb_wh_rvp(atid, pxct, atrt, this);
    assert (frvp);
    
    // 3. generate and enqueue action
    // For each generated action. Decide about partition and enqueue it.
    // 3a. Borrow the corresponding action
    // 3b. Set rvp, tid, xct, and pin
    // 3c. Decide about partition
    // 3d. Enqueue it

    { // DETACH_LOCK_CS

        // The worker threads may finish up before the client detaches.
        // We want the worker threads start detaching after the client does.
        CRITICAL_SECTION(detatch_lock_cs, frvp->_detach_lock);
        upd_wh_mb_action_impl*   p_upd_wh_mb_action   = 
            _g_dora->get_upd_wh_mb_action();
        assert (p_upd_wh_mb_action);
        p_upd_wh_mb_action->set_input(atid, pxct, frvp, this, whid);
        frvp->_puwh = p_upd_wh_mb_action;

        // Start enqueueing 
        //
        // All the enqueues should appear atomic
        // That is, there should be a total order across trxs 
        // (it terms of the sequence actions are enqueued)

        {
            int mypartition = whid-1;

            // WH_PART_CS
            CRITICAL_SECTION(wh_part_cs, _g_dora->whs(mypartition)->_enqueue_lock);
            // (SF) WAREHOUSE partitions
            if (_g_dora->whs()->enqueue(p_upd_wh_mb_action, mypartition)) {
                TRACE( TRACE_DEBUG, "Problem in enqueueing UPD_WH_MB\n");
                assert (0); 
                return (RC(de_PROBLEM_ENQUEUE));
            }
            wh_part_cs.exit();
        }

        // 4. detatch self from xct
        me()->detach_xct(pxct);
        TRACE( TRACE_TRX_FLOW, "Deattached from (%d)\n", atid);

    } // DETACH_LOCK_CS    
    return (RCOK); 
}


EXIT_NAMESPACE(tpcc);
