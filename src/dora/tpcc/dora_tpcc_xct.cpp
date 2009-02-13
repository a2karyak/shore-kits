/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_tpcc_xct.cpp
 *
 *  @brief:  Declaration of the Shore DORA transactions as part of ShoreTPCCEnv
 *
 *  @author: Ippokratis Pandis, Sept 2008
 */


#include "dora/tpcc/dora_mbench.h"
#include "dora/tpcc/dora_payment.h"
#include "dora/tpcc/dora_order_status.h"
#include "dora/tpcc/dora_stock_level.h"
#include "dora/tpcc/dora_delivery.h"
#include "dora/tpcc/dora_new_order.h"
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

    register int olcnt = anoin._ol_cnt;
    register int whid  = anoin._wh_id;
    register int did   = anoin._d_id;

    // 2. Setup the final RVP
    final_nord_rvp* frvp = NewFinalNordRvp(atid,pxct,xct_id,atrt,olcnt);


    // 3. Detatch self from xct
#ifndef ONLYDORA
    me()->detach_xct(pxct);
#endif
    TRACE( TRACE_TRX_FLOW, "Detached from (%d)\n", atid);

    // 4. Setup the (Start->Final) actions
    r_wh_nord_action* r_wh_nord = NewRWhNordAction(atid,pxct,frvp,
                                                   whid,did);

    irpImpl* my_wh_part = whs()->myPart(whid-1);

    r_cust_nord_action* r_cust_nord = NewRCustNordAction(atid,pxct,frvp,
                                                         whid,did,anoin._c_id);

    irpImpl* my_cust_part = cus()->myPart(whid-1);

    // 5. Enqueue the (Start->Final) actions
    {
        // WH_PART_CS
        CRITICAL_SECTION(wh_part_cs, my_wh_part->_enqueue_lock);

        if (my_wh_part->enqueue(r_wh_nord)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing R_WH_NORD\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }

        // CUST_PART_CS
        CRITICAL_SECTION(cust_part_cs, my_cust_part->_enqueue_lock);
        wh_part_cs.exit();

        if (my_cust_part->enqueue(r_cust_nord)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing R_CUST_NORD\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }
    }


    // Enqueue the (Start->Midway) actions

    // 6. Setup the next RVP
    midway_nord_rvp* midrvp = NewMidwayNordRvp(atid,pxct,xct_id,frvp,anoin);    

    // 7. Setup the UPD_DISTR action
    upd_dist_nord_action* upd_dist_nord = NewUpdDistNordAction(atid,pxct,midrvp,
                                                               whid,did);

    irpImpl* my_dist_part = dis()->myPart(whid-1);

    // 8. Enqueue the UPD_DISTR action
    {
        // DIST_PART_CS
        CRITICAL_SECTION(dist_part_cs, my_dist_part->_enqueue_lock);

        if (my_dist_part->enqueue(upd_dist_nord)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing UPD_DIST_NORD\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }
    }


    // 8. Setup and Enqueue the OLCNT R_ITEM and UPD_STO actions

    for (int i=0;i<olcnt;i++) {
                
        // 8a. Generate the actions
        r_item_nord_action* r_item_nord = NewRItemNordAction(atid,pxct,midrvp,whid,did,i);
        upd_sto_nord_action* upd_sto_nord = NewUpdStoNordAction(atid,pxct,midrvp,whid,did,i);        

        {
            irpImpl* my_item_part = ite()->myPart(whid-1);
            irpImpl* my_sto_part = sto()->myPart(whid-1);

            // ITEM_PART_CS
            CRITICAL_SECTION(item_part_cs, my_item_part->_enqueue_lock);

            if (my_item_part->enqueue(r_item_nord)) {
                TRACE( TRACE_DEBUG, "Problem in enqueueing R_ITEM_NORD-%d\n", i);
                assert (0); 
                return (RC(de_PROBLEM_ENQUEUE));
            }

            // STO_PART_CS
            CRITICAL_SECTION(sto_part_cs, my_sto_part->_enqueue_lock);
            item_part_cs.exit();

            if (my_sto_part->enqueue(upd_sto_nord)) {
                TRACE( TRACE_DEBUG, "Problem in enqueueing UPD_STO_NORD-%d\n", i);
                assert (0); 
                return (RC(de_PROBLEM_ENQUEUE));
            }
        }
    }

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
    midway_pay_rvp* rvp = NewMidwayPayRvp(atid,pxct,xct_id,atrt,apin);
    
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


    // 2. Setup the next and final RVP
    // PH1 consists of 2 packets
    final_ordst_rvp* rvp = NewFinalOrdStRvp(atid,pxct,xct_id,atrt);


    // 3. Do all the possible secondary index probes
            
    row_impl<customer_t>* prcust = _pcustomer_man->get_tuple();
    assert (prcust);

    row_impl<order_t>* prord = _porder_man->get_tuple();
    assert (prord);

    rep_row_t areprow(_pcustomer_man->ts());
    areprow.set(_pcustomer_desc->maxsize()); 

    prcust->_rep = &areprow;
    prord->_rep = &areprow;

    rep_row_t lowrep(_pcustomer_man->ts());
    rep_row_t highrep(_pcustomer_man->ts());
    lowrep.set(_pcustomer_desc->maxsize()); 
    highrep.set(_pcustomer_desc->maxsize()); 

    w_rc_t e = RCOK;
    bool bAbort = false;

    register int w_id = aordstin._wh_id;
    register int d_id = aordstin._d_id;

    tpcc_order_tuple aorder;
    

    r_cust_ordst_action* ordst_r_cust = NULL;
    r_ol_ordst_action* ordst_r_ol = NULL;

    { // make gotos safe 

        // 3. Determine the customer. 
        //    A probe to the secondary index may be needed.    

        // 3a. select customer based on name
        if (aordstin._c_id == 0) {

            /* SELECT  c_id, c_first
             * FROM customer
             * WHERE c_last = :c_last AND c_w_id = :w_id AND c_d_id = :d_id
             * ORDER BY c_first
             *
             * plan: index only scan on "C_NAME_INDEX"
             */

            assert (aordstin._c_select <= 60);
            assert (aordstin._c_last);

#ifndef ONLYDORA
            guard<index_scan_iter_impl<customer_t> > c_iter;
            {
                index_scan_iter_impl<customer_t>* tmp_c_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d ORDST:cust-iter-by-name-idx\n", xct_id);

                e = _pcustomer_man->cust_get_iter_by_index(_pssm, tmp_c_iter, prcust,
                                                           lowrep, highrep,
                                                           w_id, d_id, aordstin._c_last);
                c_iter = tmp_c_iter;
                if (e.is_error()) { bAbort = true; goto done; }
            }

            vector<int> v_c_id;
            int  a_c_id;
            int  count = 0;
            bool eof;

            e = c_iter->next(_pssm, eof, *prcust);
            if (e.is_error()) { bAbort = true; goto done; }
            while (!eof) {
                // push the retrieved customer id to the vector
                ++count;
                prcust->get_value(0, a_c_id);

                TRACE( TRACE_TRX_FLOW, "App: %d ORDST:cust-iter-next (%d)\n", 
                       xct_id, a_c_id);
                e = c_iter->next(_pssm, eof, *prcust);
                if (e.is_error()) { bAbort = true; goto done; }
            }
            
            // 3b. find the customer id in the middle of the list
            aordstin._c_id = v_c_id[(count+1)/2-1];
#endif
        }
        assert (aordstin._c_id>0);


        // 4. Retrieve the last order of this customer

        /* SELECT o_id, o_entry_d, o_carrier_id
         * FROM orders
         * WHERE o_w_id = :w_id AND o_d_id = :d_id AND o_c_id = :o_c_id
         * ORDER BY o_id DESC
         *
         * plan: index scan on "O_CUST_INDEX"
         */
     
        guard<index_scan_iter_impl<order_t> > o_iter;
        {
            index_scan_iter_impl<order_t>* tmp_o_iter;
            TRACE( TRACE_TRX_FLOW, "App: %d ORDST:ord-iter-by-idx\n", xct_id);
            e = _porder_man->ord_get_iter_by_index(_pssm, tmp_o_iter, prord,
                                                   lowrep, highrep,
                                                   w_id, d_id, aordstin._c_id);
            o_iter = tmp_o_iter;
            if (e.is_error()) { bAbort = true; goto done; }
        }

        bool eof;

        e = o_iter->next(_pssm, eof, *prord);
        if (e.is_error()) { bAbort = true; goto done; }
        while (!eof) {
            prord->get_value(0, aorder.O_ID);
            prord->get_value(4, aorder.O_ENTRY_D);
            prord->get_value(5, aorder.O_CARRIER_ID);
            prord->get_value(6, aorder.O_OL_CNT);

            //        rord.print_tuple();

            e = o_iter->next(_pssm, eof, *prord);
            if (e.is_error()) { bAbort = true; goto done; }
        }
     
        // we should have retrieved a valid id and ol_cnt for the order               
        assert (aorder.O_ID);
        assert (aorder.O_OL_CNT);

    } // goto


    // Now it is ready to create and enqueue the 2 actions

    
    // 5. Generate the actions
    ordst_r_cust = NewRCustOrdStAction(atid,pxct,rvp,aordstin);
    ordst_r_ol = NewROlOrdStAction(atid,pxct,rvp,aordstin,aorder);
    

    // 6. Detatch self from xct

#ifndef ONLYDORA
    me()->detach_xct(pxct);
#endif

    TRACE( TRACE_TRX_FLOW, "Detached from (%d)\n", atid);


    // For each action
    // 7a. Decide about partition
    // 7b. Enqueue
    //
    // All the enqueues should appear atomic
    // That is, there should be a total order across trxs 
    // (it terms of the sequence actions are enqueued)

    {
        int wh = aordstin._wh_id-1;

        // first, figure out to which partitions to enqueue
        irpImpl* my_cust_part = cus()->myPart(wh);
        irpImpl* my_ol_part = oli()->myPart(wh);

        // then, start enqueueing

        // CUST_PART_CS
        CRITICAL_SECTION(cust_part_cs, my_cust_part->_enqueue_lock);
            
        if (my_cust_part->enqueue(ordst_r_cust)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing ORDST_R_CUST\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }

        // OL_PART_CS
        CRITICAL_SECTION(ol_part_cs, my_ol_part->_enqueue_lock);
        cust_part_cs.exit();

        if (my_ol_part->enqueue(ordst_r_ol)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing ORDST_R_OL\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }
    }


done:

    // 8. Return the tuples to the cache
    _pcustomer_man->give_tuple(prcust);
    _porder_man->give_tuple(prord);  

    // 9. if something went wrong no action has been enqueued yet.
    //    Therefore, it is responsibility of the dispatcher to execute the 
    //    RVP abort code, in order to release the locks acquired during the 
    //    secondary indexes probes.
    if (bAbort) {
        
        TRACE( TRACE_TRX_FLOW, "Dispatcher needs to abort (%d)\n", atid);

        rvp->abort();
        e = rvp->run();
        if (e.is_error()) {
            TRACE( TRACE_ALWAYS, "Problem running rvp for xct (%d) [0x%x]\n",
                   atid, e.err_num());
        }
            
        // delete rvp
        rvp->giveback();
        rvp = NULL;
    }

    return (e); 
}


w_rc_t DoraTPCCEnv::dora_delivery(const int xct_id, 
                                  delivery_input_t& adelin,
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

    // 2. Setup the final RVP
    final_del_rvp* frvp = NewFinalDelRvp(atid,pxct,xct_id,atrt);


    // 3. Detatch self from xct
#ifndef ONLYDORA
    me()->detach_xct(pxct);
#endif
    TRACE( TRACE_TRX_FLOW, "Detached from (%d)\n", atid);

    // 4. Setup the next RVP and actions
    // PH1 consists of DISTRICTS_PER_WAREHOUSE actions
    for (int i=0;i<DISTRICTS_PER_WAREHOUSE;i++) {
        // 4a. Generate an RVP
        mid1_del_rvp* rvp = NewMid1DelRvp(atid,pxct,xct_id,frvp,i,adelin);
    
        // 4b. Generate an action
        del_nord_del_action* del_del_nord = NewDelNordDelAction(atid,pxct,rvp,adelin,i);

        // For each action
        // 5a. Decide about partition
        // 5b. Enqueue
        //
        // All the enqueues should appear atomic
        // That is, there should be a total order across trxs 
        // (it terms of the sequence actions are enqueued)

        {
            int wh = adelin._wh_id-1;

            // first, figure out to which partitions to enqueue
            irpImpl* my_nord_part = nor()->myPart(wh);

            // then, start enqueueing

            // NORD_PART_CS
            CRITICAL_SECTION(nord_part_cs, my_nord_part->_enqueue_lock);

            if (my_nord_part->enqueue(del_del_nord)) {
                TRACE( TRACE_DEBUG, "Problem in enqueueing DEL_DEL_NORD-%d\n", i);
                assert (0); 
                return (RC(de_PROBLEM_ENQUEUE));
            }
        }
    }

    return (RCOK); 
}


w_rc_t DoraTPCCEnv::dora_stock_level(const int xct_id, 
                                     stock_level_input_t& astoin,
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
    // PH1 consists of 1 packet
    mid1_stock_rvp* rvp = NewMid1StockRvp(atid,pxct,xct_id,atrt,astoin);
    
    // 3. Generate the action
    r_dist_stock_action* stock_r_dist = NewRDistStockAction(atid,pxct,rvp,astoin);

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
        int wh = astoin._wh_id-1;

        // first, figure out to which partitions to enqueue
        irpImpl* my_dist_part = dis()->myPart(wh);

        // then, start enqueueing

        // DIS_PART_CS
        CRITICAL_SECTION(dis_part_cs, my_dist_part->_enqueue_lock);

        if (my_dist_part->enqueue(stock_r_dist)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing STOCK_R_DIST\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }
    }

    return (RCOK); 
}



// --- without input specified --- //

w_rc_t DoraTPCCEnv::dora_new_order(const int xct_id, 
                                   trx_result_tuple_t& atrt,
                                   int specificWH)
{
    new_order_input_t noin = create_new_order_input(_queried_factor, 
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
