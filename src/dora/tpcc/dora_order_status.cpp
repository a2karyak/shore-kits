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
// (1) final_ordst_rvp
//


/******************************************************************** 
 *
 * ORDER STATUS FINAL RVP
 *
 ********************************************************************/

w_rc_t final_ordst_rvp::run() 
{
    return (_run(_ptpccenv)); 
}

void final_ordst_rvp::upd_committed_stats() 
{
    _ptpccenv->_inc_order_status_att();
}                     

void final_ordst_rvp::upd_aborted_stats() 
{
    _ptpccenv->_inc_order_status_failed();
}                     



/******************************************************************** 
 *
 * ORDER STATUS TPC-C DORA ACTIONS
 *
 * (1) READ-CUSTOMER
 * (2) READ-ORDERLINES
 *
 ********************************************************************/

w_rc_t r_cust_ordst_action::trx_exec() 
{
    w_assert3 (_ptpccenv);

    // get table tuple from the cache
    row_impl<customer_t>* prcust = _ptpccenv->customer_man()->get_tuple();
    w_assert3 (prcust);
    rep_row_t areprow(_ptpccenv->customer_man()->ts());
    areprow.set(_ptpccenv->customer()->maxsize()); 
    prcust->_rep = &areprow;

    w_rc_t e = RCOK;

    register int w_id = _ordstin._wh_id;
    register int d_id = _ordstin._d_id;
    register int c_id = _ordstin._c_id;

    { // make gotos safe

        /* 1. probe the customer */

        /* SELECT c_first, c_middle, c_last, c_balance
         * FROM customer
         * WHERE c_id = :c_id AND c_w_id = :w_id AND c_d_id = :d_id
         *
         * plan: index probe on "C_INDEX"
         */

        TRACE( TRACE_TRX_FLOW, 
               "App: %d ORDST:cust-idx-probe-nl (%d) (%d) (%d)\n", 
               _tid, w_id, d_id, c_id);

#ifndef ONLYDORA
        e = _ptpccenv->customer_man()->cust_index_probe_nl(_ptpccenv->db(), prcust, 
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
    _ptpccenv->customer_man()->give_tuple(prcust);
    return (e);
}

w_rc_t r_ol_ordst_action::trx_exec() 
{
    w_assert3 (_ptpccenv);

    // get table tuple from the cache

    row_impl<order_line_t>* prol = _ptpccenv->order_line_man()->get_tuple();
    w_assert3(prol);

    rep_row_t areprow(_ptpccenv->order_line_man()->ts());
    areprow.set(_ptpccenv->order_line()->maxsize()); 
    prol->_rep = &areprow;

    rep_row_t lowrep(_ptpccenv->order_line_man()->ts());
    rep_row_t highrep(_ptpccenv->order_line_man()->ts());

    tpcc_orderline_tuple* porderlines = NULL;

    w_rc_t e = RCOK;

    register int w_id = _ordstin._wh_id;
    register int d_id = _ordstin._d_id;
    register int o_id = _aorder.O_ID;


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
            e = _ptpccenv->order_line_man()->ol_get_probe_iter_by_index_nl(_ptpccenv->db(), 
                                                                          tmp_ol_iter, prol,
                                                                          lowrep, highrep,
                                                                          w_id, d_id, o_id);
            ol_iter = tmp_ol_iter;
            if (e.is_error()) { goto done; }
        }
        
        porderlines = new tpcc_orderline_tuple[_aorder.O_OL_CNT];
        int i=0;
        bool eof;

        e = ol_iter->next(_ptpccenv->db(), eof, *prol);
        if (e.is_error()) { goto done; }
        while (!eof) {
            prol->get_value(4, porderlines[i].OL_I_ID);
            prol->get_value(5, porderlines[i].OL_SUPPLY_W_ID);
            prol->get_value(6, porderlines[i].OL_DELIVERY_D);
            prol->get_value(7, porderlines[i].OL_QUANTITY);
            prol->get_value(8, porderlines[i].OL_AMOUNT);

#ifdef PRINT_TRX_RESULTS
            TRACE( TRACE_TRX_FLOW, "(%d) %d - %d - %d - %d\n", 
                   i, porderlines[i].OL_I_ID, porderlines[i].OL_SUPPLY_W_ID,
                   porderlines[i].OL_QUANTITY, porderlines[i].OL_AMOUNT);
#endif

            i++;
            e = ol_iter->next(_ptpccenv->db(), eof, *prol);
            if (e.is_error()) { goto done; }
        }
#endif

    } // goto 

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prol->print_tuple();
#endif

done:
    // give back the tuple
    _ptpccenv->order_line_man()->give_tuple(prol);   
    if (porderlines) delete [] porderlines;
    return (e);
}


EXIT_NAMESPACE(dora);
