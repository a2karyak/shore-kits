/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_order_status.cpp
 *
 *  @brief:  DORA TPC-C ORDER STATUS
 *
 *  @note:   Implementation of RVPs and Actions that synthesize 
 *           the TPC-C order status trx according to DORA
 *
 *  @author: Ippokratis Pandis, Jan 2009
 */

#include "dora/tpcc/dora_order_status.h"
#include "dora/tpcc/dora_tpcc.h"


using namespace dora;
using namespace shore;
using namespace tpcc;



ENTER_NAMESPACE(dora);

//#define PRINT_TRX_RESULTS

//
// RVPS
//
// (1) mid1_ordst_rvp
// (2) mid2_ordst_rvp
// (3) final_ordst_rvp
//


DEFINE_DORA_FINAL_RVP_CLASS(final_ordst_rvp,order_status);


/******************************************************************** 
 *
 * (1) mid1_ordst_rvp
 *
 * Enqueues the read-order action
 *
 ********************************************************************/

w_rc_t mid1_ordst_rvp::run()
{
#ifndef ONLYDORA
    assert (_xct);
#endif

    // 1. Setup the next RVP
    mid2_ordst_rvp* mid2_rvp = _penv->new_mid2_ordst_rvp(_tid,_xct,_xct_id,_result,_in,_actions,_bWake);

    // 2. Generate the action
    r_ord_ordst_action* r_ord = _penv->new_r_ord_ordst_action(_tid,_xct,mid2_rvp,_in);

    TRACE( TRACE_TRX_FLOW, "Next phase (%d)\n", _tid);    
    typedef range_partition_impl<int>   irpImpl; 

    // 3a. Decide about partition
    // 3b. Enqueue

    {        
        register int wh = _in._wh_id - 1;
        irpImpl* my_ord_part = _penv->ord()->myPart(wh);

        // ORD_PART_CS
        CRITICAL_SECTION(ord_part_cs, my_ord_part->_enqueue_lock);
        if (my_ord_part->enqueue(r_ord,_bWake)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing ORDST_R_ORD\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }
    }
    return (RCOK);
}



/******************************************************************** 
 *
 * (1) mid2_ordst_rvp
 *
 * Enqueues the read-orderline action
 *
 ********************************************************************/

w_rc_t mid2_ordst_rvp::run()
{
#ifndef ONLYDORA
    assert (_xct);
#endif

    // 1. Setup the next RVP
    final_ordst_rvp* frvp = _penv->new_final_ordst_rvp(_tid,_xct,_xct_id,_result,_actions);

    // 2. Generate the action
    r_ol_ordst_action* r_ol = _penv->new_r_ol_ordst_action(_tid,_xct,frvp,_in);

    TRACE( TRACE_TRX_FLOW, "Next phase (%d)\n", _tid);    
    typedef range_partition_impl<int>   irpImpl; 

    // 3a. Decide about partition
    // 3b. Enqueue

    {        
        register int wh = _in._wh_id - 1;
        irpImpl* my_oli_part = _penv->oli()->myPart(wh);

        // OLI_PART_CS
        CRITICAL_SECTION(oli_part_cs, my_oli_part->_enqueue_lock);
        if (my_oli_part->enqueue(r_ol,_bWake)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing ORDST_R_OL\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }
    }
    return (RCOK);
}




/******************************************************************** 
 *
 * ORDER STATUS TPC-C DORA ACTIONS
 *
 * (1) READ-CUSTOMER
 * (2) READ-ORDER
 * (2) READ-ORDERLINES
 *
 ********************************************************************/

#warning Only 2 fields (WH,DI) determine the CUSTOMER table accesses! Not 3.

void r_cust_ordst_action::calc_keys()
{
    set_read_only();
    _down.push_back(_in._wh_id);
    _down.push_back(_in._d_id);
    _up.push_back(_in._wh_id);
    _up.push_back(_in._d_id);
}


w_rc_t r_cust_ordst_action::trx_exec() 
{
    assert (_penv);
    w_rc_t e = RCOK;

    // get table tuple from the cache
    row_impl<customer_t>* prcust = _penv->customer_man()->get_tuple();
    assert (prcust);

    rep_row_t areprow(_penv->customer_man()->ts());
    areprow.set(_penv->customer_desc()->maxsize()); 
    prcust->_rep = &areprow;

    rep_row_t lowrep(_penv->customer_man()->ts());
    rep_row_t highrep(_penv->customer_man()->ts());
    lowrep.set(_penv->customer_desc()->maxsize()); 
    highrep.set(_penv->customer_desc()->maxsize()); 

    register int w_id = _in._wh_id;
    register int d_id = _in._d_id;
    register int c_id = _in._c_id;

    { // make gotos safe

        // 1. Determine the customer. 
        //    A probe to the secondary index may be needed.    

        // 1a. select customer based on name
        if (_in._c_id == 0) {

            /* SELECT  c_id, c_first
             * FROM customer
             * WHERE c_last = :c_last AND c_w_id = :w_id AND c_d_id = :d_id
             * ORDER BY c_first
             *
             * plan: index only scan on "C_NAME_INDEX"
             */

            assert (_in._c_select <= 60);
            assert (_in._c_last);

#ifndef ONLYDORA
            guard<index_scan_iter_impl<customer_t> > c_iter;
            {
                index_scan_iter_impl<customer_t>* tmp_c_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d ORDST:cust-iter-by-name-idx-nl\n", _tid);

                e = _penv->customer_man()->cust_get_iter_by_index_nl(_penv->db(), tmp_c_iter, prcust,
                                                                     lowrep, highrep,
                                                                     w_id, d_id, _in._c_last);
                c_iter = tmp_c_iter;
                if (e.is_error()) { goto done; }
            }

            vector<int> v_c_id;
            int  a_c_id;
            int  count = 0;
            bool eof;

            e = c_iter->next(_penv->db(), eof, *prcust);
            if (e.is_error()) { goto done; }
            while (!eof) {
                // push the retrieved customer id to the vector
                ++count;
                prcust->get_value(0, a_c_id);

                TRACE( TRACE_TRX_FLOW, "App: %d ORDST:cust-iter-next (%d)\n", 
                       _tid, a_c_id);
                e = c_iter->next(_penv->db(), eof, *prcust);
                if (e.is_error()) { goto done; }
            }
            
            // 1b. find the customer id in the middle of the list
            _in._c_id = v_c_id[(count+1)/2-1];
#endif
        }
        assert (_in._c_id>0);


        /* 2. probe the customer */

        /* SELECT c_first, c_middle, c_last, c_balance
         * FROM customer
         * WHERE c_id = :c_id AND c_w_id = :w_id AND c_d_id = :d_id
         *
         * plan: index probe on "C_INDEX"
         */

        TRACE( TRACE_TRX_FLOW, 
               "App: %d ORDST:cust-idx-nl (%d) (%d) (%d)\n", 
               _tid, w_id, d_id, c_id);

#ifndef ONLYDORA
        e = _penv->customer_man()->cust_index_probe_nl(_penv->db(), prcust, 
                                                       w_id, d_id, c_id);
#endif

        if (e.is_error()) { goto done; }
            
        tpcc_customer_tuple acust;
        prcust->get_value(3,  acust.C_FIRST, 17);
        prcust->get_value(4,  acust.C_MIDDLE, 3);
        prcust->get_value(5,  acust.C_LAST, 17);
        prcust->get_value(16, acust.C_BALANCE);

    } // goto

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prcust->print_tuple();
#endif

done:
    // give back the tuple
    _penv->customer_man()->give_tuple(prcust);
    return (e);
}




#warning Only 2 fields (WH,DI) determine the ORDER table accesses! Not 3.

void r_ord_ordst_action::calc_keys()
{
    set_read_only();
    _down.push_back(_in._wh_id);
    _down.push_back(_in._d_id);
    _up.push_back(_in._wh_id);
    _up.push_back(_in._d_id);
}


w_rc_t r_ord_ordst_action::trx_exec() 
{
    assert (_penv);
    w_rc_t e = RCOK;

    // get table tuple from the cache
    row_impl<order_t>* prord = _penv->order_man()->get_tuple();
    assert (prord);

    rep_row_t areprow(_penv->order_man()->ts());
    areprow.set(_penv->order_desc()->maxsize()); 
    prord->_rep = &areprow;

    rep_row_t lowrep(_penv->order_man()->ts());
    rep_row_t highrep(_penv->order_man()->ts());
    lowrep.set(_penv->order_desc()->maxsize()); 
    highrep.set(_penv->order_desc()->maxsize()); 

    register int w_id = _in._wh_id;
    register int d_id = _in._d_id;
    register int c_id = _in._c_id;

    { // make gotos safe

        // 1. Retrieve the last order of a specific customer

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
            TRACE( TRACE_TRX_FLOW, "App: %d ORDST:ord-iter-by-idx-nl\n", _tid);
            e = _penv->order_man()->ord_get_iter_by_index_nl(_penv->db(), tmp_o_iter, prord,
                                                             lowrep, highrep,
                                                             w_id, d_id, c_id);
            o_iter = tmp_o_iter;
            if (e.is_error()) { goto done; }
        }

        tpcc_order_tuple aorder;
        bool eof;

        e = o_iter->next(_penv->db(), eof, *prord);
        if (e.is_error()) { goto done; }
        while (!eof) {
            prord->get_value(0, aorder.O_ID);
            prord->get_value(4, aorder.O_ENTRY_D);
            prord->get_value(5, aorder.O_CARRIER_ID);
            prord->get_value(6, aorder.O_OL_CNT);

            e = o_iter->next(_penv->db(), eof, *prord);
            if (e.is_error()) { goto done; }
        }
     
        // we should have retrieved a valid id and ol_cnt for the order               
        assert (aorder.O_ID);
        assert (aorder.O_OL_CNT);

        TRACE( TRACE_TRX_FLOW, "App: %d ORDST: (%d) (%d)\n", _tid, aorder.O_ID, aorder.O_OL_CNT);

        // need to update the RVP
        _prvp->_in._o_id = aorder.O_ID;
        _prvp->_in._o_ol_cnt = aorder.O_OL_CNT;
        
    } // goto

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prord->print_tuple();
#endif

done:
    // give back the tuple
    _penv->order_man()->give_tuple(prord);
    return (e);
}



#warning Only 2 fields (WH,DI) determine the ORDERLINE table accesses! Not 3.

void r_ol_ordst_action::calc_keys()
{
    set_read_only();
    _down.push_back(_in._wh_id);
    _down.push_back(_in._d_id);
    _up.push_back(_in._wh_id);
    _up.push_back(_in._d_id);
}


w_rc_t r_ol_ordst_action::trx_exec() 
{
    assert (_penv);

    // get table tuple from the cache

    row_impl<order_line_t>* prol = _penv->order_line_man()->get_tuple();
    assert(prol);

    rep_row_t areprow(_penv->order_line_man()->ts());
    areprow.set(_penv->order_line_desc()->maxsize()); 
    prol->_rep = &areprow;

    rep_row_t lowrep(_penv->order_line_man()->ts());
    rep_row_t highrep(_penv->order_line_man()->ts());

    tpcc_orderline_tuple* porderlines = NULL;

    w_rc_t e = RCOK;

    register int w_id = _in._wh_id;
    register int d_id = _in._d_id;
    register int o_id = _in._o_id;

    { // make gotos safe 
     
        /* 3. retrieve all the orderlines that correspond to the last order */
     
        /* SELECT ol_i_id, ol_supply_w_id, ol_quantity, ol_amount, ol_delivery_d 
         * FROM order_line 
         * WHERE ol_w_id = :H00003 AND ol_d_id = :H00004 AND ol_o_id = :H00016 
         *
         * plan: index scan on "OL_INDEX"
         */

#ifndef ONLYDORA
        guard<index_scan_iter_impl<order_line_t> > ol_iter;
        {
            index_scan_iter_impl<order_line_t>* tmp_ol_iter;
            TRACE( TRACE_TRX_FLOW, "App: %d ORDST:ol-iter-by-idx-nl\n", _tid);
            e = _penv->order_line_man()->ol_get_probe_iter_by_index_nl(_penv->db(), 
                                                                       tmp_ol_iter, prol,
                                                                       lowrep, highrep,
                                                                       w_id, d_id, o_id);
            ol_iter = tmp_ol_iter;
            if (e.is_error()) { goto done; }
        }
        
        porderlines = new tpcc_orderline_tuple[_in._o_ol_cnt];
        int i=0;
        bool eof;

        e = ol_iter->next(_penv->db(), eof, *prol);
        if (e.is_error()) { goto done; }
        while (!eof) {
            prol->get_value(4, porderlines[i].OL_I_ID);
            prol->get_value(5, porderlines[i].OL_SUPPLY_W_ID);
            prol->get_value(6, porderlines[i].OL_DELIVERY_D);
            prol->get_value(7, porderlines[i].OL_QUANTITY);
            prol->get_value(8, porderlines[i].OL_AMOUNT);

#ifdef PRINT_TRX_RESULTS
            TRACE( TRACE_TRX_FLOW, "App: %d ORDST: (%d) %d - %d - %d - %d\n", 
                   _tid, 
                   i, porderlines[i].OL_I_ID, porderlines[i].OL_SUPPLY_W_ID,
                   porderlines[i].OL_QUANTITY, porderlines[i].OL_AMOUNT);
#endif

            i++;
            e = ol_iter->next(_penv->db(), eof, *prol);
            if (e.is_error()) { goto done; }
        }
        TRACE( TRACE_TRX_FLOW, "App: %d ORDST: found (%d)\n", _tid, i);
#endif

    } // goto 

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prol->print_tuple();
#endif

done:
    // give back the tuple
    _penv->order_line_man()->give_tuple(prol);   
    if (porderlines) delete [] porderlines;
    return (e);
}


EXIT_NAMESPACE(dora);