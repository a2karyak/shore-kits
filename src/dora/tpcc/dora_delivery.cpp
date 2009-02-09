/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_delivery.cpp
 *
 *  @brief:  DORA TPC-C DELIVERY
 *
 *  @note:   Implementation of RVPs and Actions that synthesize 
 *           the TPC-C Delivery trx according to DORA
 *
 *  @author: Ippokratis Pandis, Jan 2009
 */

#include "dora/tpcc/dora_delivery.h"
#include "dora/tpcc/dora_tpcc.h"


using namespace dora;
using namespace shore;
using namespace tpcc;



ENTER_NAMESPACE(dora);


//
// RVPS
//
// (1) mid1_del_rvp
// (2) mid2_del_rvp
// (3) final_del_rvp
//


/******************************************************************** 
 *
 * DELIVERY MID-1 RVP - enqueues the UPD(ORD) - UPD(OL) actions
 *
 ********************************************************************/

w_rc_t mid1_del_rvp::run() 
{
    // 1. Setup the next RVP
#ifndef ONLYDORA
    assert (_xct);
#endif

    mid2_del_rvp* rvp2 = _ptpccenv->NewMid2DelRvp(_tid,_xct,_xct_id,_final_rvp,_d_id,_din,_actions);

    // 2. Generate and enqueue actions
    upd_ord_del_action* del_upd_ord = _ptpccenv->NewUpdOrdDelAction(_tid,_xct,rvp2,_din,_d_id,_o_id);
    upd_oline_del_action* del_upd_oline = _ptpccenv->NewUpdOlineDelAction(_tid,_xct,rvp2,_din,_d_id,_o_id);
    typedef range_partition_impl<int>   irpImpl; 

    {
        int wh = _din._wh_id-1;
        irpImpl* my_ord_part = _ptpccenv->ord()->myPart(wh);
        irpImpl* my_oline_part = _ptpccenv->oli()->myPart(wh);

        TRACE( TRACE_TRX_FLOW, "Next phase (%d-%d)\n", _tid, _d_id);
        
        // ORD_PART_CS
        CRITICAL_SECTION(ord_part_cs, my_ord_part->_enqueue_lock);
        if (my_ord_part->enqueue(del_upd_ord)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing DEL_UPD_ORD\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }
        
        // OL_PART_CS
        CRITICAL_SECTION(ol_part_cs, my_oline_part->_enqueue_lock);
        ord_part_cs.exit();
        if (my_oline_part->enqueue(del_upd_oline)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing DEL_UPD_OL\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }        
    }

    return (RCOK);
}



/******************************************************************** 
 *
 * DELIVERY MID-2 RVP - enqueues the UPD(CUST) action
 *
 ********************************************************************/

w_rc_t mid2_del_rvp::run() 
{
    // 1. the next rvp (final_rvp) is already set
    //    so it needs to only append the actions
    assert (_final_rvp);
    _final_rvp->append_actions(_actions);

    // 2. Generate and enqueue action
    upd_cust_del_action* del_upd_cust = _ptpccenv->NewUpdCustDelAction(_tid,_xct,_final_rvp,_din,_d_id,_c_id,_amount);
    typedef range_partition_impl<int>   irpImpl; 

    {
        int wh = _din._wh_id-1;
        irpImpl* my_cust_part = _ptpccenv->cus()->myPart(wh);

        TRACE( TRACE_TRX_FLOW, "Next phase (%d-%d)\n", _tid, _d_id);
        
        // CUST_PART_CS
        CRITICAL_SECTION(cust_part_cs, my_cust_part->_enqueue_lock);
        if (my_cust_part->enqueue(del_upd_cust)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing DEL_UPD_CUST\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }
    }

    return (RCOK);
}




/******************************************************************** 
 *
 * DELIVERY FINAL RVP
 *
 ********************************************************************/

w_rc_t final_del_rvp::run() {
    assert (_ptpccenv); 
    return (_run(_ptpccenv)); 
}

void final_del_rvp::upd_committed_stats() 
{
    _ptpccenv->_inc_deliv_att();
}                     

void final_del_rvp::upd_aborted_stats() 
{
    _ptpccenv->_inc_deliv_failed();
}                     



/******************************************************************** 
 *
 * DELIVERY TPC-C DORA ACTIONS
 *
 * (1) DELETE-NEW_ORDER
 * (2) UPDATE-ORDER
 * (3) UPDATE-ORDERLINE(s)
 * (4) UPDATE-CUSTOMER
 *
 ********************************************************************/

w_rc_t del_nord_del_action::trx_exec() 
{
    w_assert3 (_ptpccenv);

    // get table tuple from the cache
    row_impl<new_order_t>* prno = _ptpccenv->new_order_man()->get_tuple();
    w_assert3 (prno);

    rep_row_t areprow(_ptpccenv->new_order_man()->ts());
    areprow.set(_ptpccenv->new_order()->maxsize()); 
    prno->_rep = &areprow;

    rep_row_t lowrep(_ptpccenv->new_order_man()->ts());
    rep_row_t highrep(_ptpccenv->new_order_man()->ts());

    // allocate space for the biggest of the (new_order) and (orderline)
    // table representations
    lowrep.set(_ptpccenv->new_order()->maxsize()); 
    highrep.set(_ptpccenv->new_order()->maxsize()); 

    w_rc_t e = RCOK;

    { // make gotos safe

        /* 1. Get the new_order of the district, with the min value */

        /* SELECT MIN(no_o_id) INTO :no_o_id:no_o_id_i
         * FROM new_order
         * WHERE no_d_id = :d_id AND no_w_id = :w_id
         *
         * plan: index scan on "NO_INDEX"
         */
        TRACE( TRACE_TRX_FLOW, "App: %d DEL:nord-iter-by-idx-nl (%d) (%d)\n", 
               _tid, _din._wh_id, _d_id);

#ifndef ONLYDORA    
        guard<index_scan_iter_impl<new_order_t> > no_iter;
        {
            index_scan_iter_impl<new_order_t>* tmp_no_iter;
            e = _ptpccenv->new_order_man()->no_get_iter_by_index_nl(_ptpccenv->db(), 
                                                                    tmp_no_iter, 
                                                                    prno, 
                                                                    lowrep, highrep,
                                                                    _din._wh_id, 
                                                                    _d_id);
            no_iter = tmp_no_iter;
            if (e.is_error()) { goto done; }
        }

        bool eof;

        // iterate over all new_orders and load their no_o_ids to the sort buffer
        e = no_iter->next(_ptpccenv->db(), eof, *prno);
        if (e.is_error()) { goto done; }

        if (eof) { goto done; } // skip this district

        int no_o_id;
        prno->get_value(0, no_o_id);
        assert (no_o_id);
        _pmid1_rvp->set_o_id(no_o_id);


        /* 2. Delete the retrieved new order */        

        /* DELETE FROM new_order
         * WHERE no_w_id = :w_id AND no_d_id = :d_id AND no_o_id = :no_o_id
         *
         * plan: index scan on "NO_INDEX"
         */

        TRACE( TRACE_TRX_FLOW, "App: %d DEL:nord-delete-by-index-nl (%d) (%d) (%d)\n", 
               _tid, _din._wh_id, _d_id, no_o_id);

        e = _ptpccenv->new_order_man()->no_delete_by_index_nl(_ptpccenv->db(), 
                                                              prno, 
                                                              _din._wh_id, 
                                                              _d_id, 
                                                              no_o_id);
        if (e.is_error()) { goto done; }
#endif

    } // goto

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prno->print_tuple();
#endif

done:
    // give back the tuple
    _ptpccenv->new_order_man()->give_tuple(prno);
    return (e);
}



w_rc_t upd_ord_del_action::trx_exec() 
{
    w_assert3 (_ptpccenv);

    // get table tuple from the cache
    row_impl<order_t>* prord = _ptpccenv->order_man()->get_tuple();
    w_assert3 (prord);

    rep_row_t areprow(_ptpccenv->order_man()->ts());
    areprow.set(_ptpccenv->order()->maxsize()); 
    prord->_rep = &areprow;

    w_rc_t e = RCOK;

    { // make gotos safe

        if (_o_id<0) { goto done; } // empty district

        /* 3a. Update the carrier for the delivered order (in the orders table) */
        /* 3b. Get the customer id of the updated order */

        /* UPDATE orders SET o_carrier_id = :o_carrier_id
         * SELECT o_c_id INTO :o_c_id FROM orders
         * WHERE o_id = :no_o_id AND o_w_id = :w_id AND o_d_id = :d_id;
         *
         * plan: index probe on "O_INDEX"
         */

        TRACE( TRACE_TRX_FLOW, "App: %d DEL:ord-idx-probe-upd (%d) (%d) (%d)\n", 
               _tid, _din._wh_id, _d_id, _o_id);
        
        prord->set_value(0, _o_id);
        prord->set_value(2, _d_id);
        prord->set_value(3, _din._wh_id);

#ifndef ONLYDORA
        e = _ptpccenv->order_man()->ord_update_carrier_by_index_nl(_ptpccenv->db(), 
                                                                   prord, 
                                                                   _din._carrier_id);
#endif

        if (e.is_error()) { goto done; }

        int  c_id;
        prord->get_value(1, c_id);

        _pmid2_rvp->set_c_id(c_id);

    } // goto

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prord->print_tuple();
#endif

done:
    // give back the tuple
    _ptpccenv->order_man()->give_tuple(prord);
    return (e);
}



w_rc_t upd_oline_del_action::trx_exec() 
{
    w_assert3 (_ptpccenv);

    // get table tuple from the cache
    row_impl<order_line_t>* prol = _ptpccenv->orderline_man()->get_tuple();
    w_assert3 (prol);

    rep_row_t areprow(_ptpccenv->orderline_man()->ts());
    areprow.set(_ptpccenv->orderline()->maxsize()); 
    prol->_rep = &areprow;

    rep_row_t lowrep(_ptpccenv->orderline_man()->ts());
    rep_row_t highrep(_ptpccenv->orderline_man()->ts());

    // allocate space for the biggest of the (new_order) and (orderline)
    // table representations
    lowrep.set(_ptpccenv->orderline()->maxsize()); 
    highrep.set(_ptpccenv->orderline()->maxsize()); 

    time_t ts_start = time(NULL);

    w_rc_t e = RCOK;

    { // make gotos safe

        if (_o_id<0) { goto done; } // empty district
        
        /* 4a. Calculate the total amount of the orders from orderlines */
        /* 4b. Update all the orderlines with the current timestamp */
           
        /* SELECT SUM(ol_amount) INTO :total_amount FROM order_line
         * UPDATE ORDER_LINE SET ol_delivery_d = :curr_tmstmp
         * WHERE ol_w_id = :w_id AND ol_d_id = :no_d_id AND ol_o_id = :no_o_id;
         *
         * plan: index scan on "OL_INDEX"
         */


        TRACE( TRACE_TRX_FLOW, 
               "App: %d DEL:ol-iter-probe-by-idx-nl (%d) (%d) (%d)\n", 
               _tid, _din._wh_id, _d_id, _o_id);

        int total_amount = 0;

#ifndef ONLYDORA
        guard<index_scan_iter_impl<order_line_t> > ol_iter;
        {
            index_scan_iter_impl<order_line_t>* tmp_ol_iter;
            e = _ptpccenv->orderline_man()->ol_get_probe_iter_by_index_nl(_ptpccenv->db(), 
                                                                          tmp_ol_iter, 
                                                                          prol, 
                                                                          lowrep, 
                                                                          highrep,
                                                                          _din._wh_id, 
                                                                           _d_id, 
                                                                          _o_id);
            ol_iter = tmp_ol_iter;
            if (e.is_error()) { goto done; }
        }
        
        // iterate over all the orderlines for the particular order
        bool eof;
        e = ol_iter->next(_ptpccenv->db(), eof, *prol);
        if (e.is_error()) { goto done; }
        while (!eof) {
            // update the total amount
            int current_amount;
            prol->get_value(8, current_amount);
            total_amount += current_amount;

            // update orderline
            prol->set_value(6, ts_start);
            e = _ptpccenv->orderline_man()->update_tuple(_ptpccenv->db(), prol, NL);
            if (e.is_error()) { goto done; }

            // go to the next orderline
            e = ol_iter->next(_ptpccenv->db(), eof, *prol);
            if (e.is_error()) { goto done; }
        }
#endif

        _pmid2_rvp->add_amount(total_amount);

    } // goto

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prol->print_tuple();
#endif

done:
    // give back the tuple
    _ptpccenv->orderline_man()->give_tuple(prol);
    return (e);
}




w_rc_t upd_cust_del_action::trx_exec() 
{
    w_assert3 (_ptpccenv);

    // get table tuple from the cache
    row_impl<customer_t>* prcust = _ptpccenv->customer_man()->get_tuple();
    w_assert3 (prcust);

    rep_row_t areprow(_ptpccenv->customer_man()->ts());
    areprow.set(_ptpccenv->customer()->maxsize()); 
    prcust->_rep = &areprow;

    w_rc_t e = RCOK;

    { // make gotos safe

        if (_c_id<0) { goto done; } // empty district


        /* 5. Update balance of the customer of the order */

        /* UPDATE customer
         * SET c_balance = c_balance + :total_amount, c_delivery_cnt = c_delivery_cnt + 1
         * WHERE c_id = :c_id AND c_w_id = :w_id AND c_d_id = :no_d_id;
         *
         * plan: index probe on "C_INDEX"
         */

        TRACE( TRACE_TRX_FLOW, "App: %d DEL:cust-idx-probe-upd-nl (%d) (%d) (%d)\n", 
               _tid, _din._wh_id, _d_id, _c_id);

#ifndef ONLYDORA
        e = _ptpccenv->customer_man()->cust_index_probe_nl(_ptpccenv->db(),
                                                           prcust, 
                                                           _din._wh_id, 
                                                           _d_id, 
                                                           _c_id);
#endif

        if (e.is_error()) { goto done; }

        double balance;
        prcust->get_value(16, balance);
        prcust->set_value(16, balance+_amount);

#ifndef ONLYDORA
        e = _ptpccenv->customer_man()->update_tuple(_ptpccenv->db(), prcust, NL);
#endif

        if (e.is_error()) { goto done; }

    } // goto

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prcust->print_tuple();
#endif

done:
    // give back the tuple
    _ptpccenv->customer_man()->give_tuple(prcust);
    return (e);
}



EXIT_NAMESPACE(dora);