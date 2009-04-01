/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_new_order.cpp
 *
 *  @brief:  DORA TPC-C NEW-ORDER
 *
 *  @note:   Implementation of RVPs and Actions that synthesize 
 *           the TPC-C NewOrder trx according to DORA
 *
 *  @author: Ippokratis Pandis, Nov 2008
 */

#include "dora/tpcc/dora_new_order.h"


using namespace dora;
using namespace shore;
using namespace tpcc;



ENTER_NAMESPACE(dora);


//
// RVPS
//
// (1) final_nord_rvp
// (2) midway_nord_rvp
//


/******************************************************************** 
 *
 * NEWORDER MIDWAY RVP - enqueues the I(ORD) - I(NORD) - OL_CNT x I(OL) actions
 *
 ********************************************************************/

w_rc_t midway_nord_rvp::run() 
{
    // 1. the next rvp (final_rvp) is already set
    //    so it needs to only append the actions
    assert (_final_rvp);
    _final_rvp->append_actions(_actions);

    register int whid    = _noin._wh_id;
    register int did     = _noin._d_id;
    register int nextoid = _noin._d_next_o_id;
    register int olcnt   = _noin._ol_cnt;

    typedef range_partition_impl<int>   irpImpl; 


    // 2. Generate and enqueue actions

    // 2a. Insert (ORD)
    ins_ord_nord_action* ins_ord_nord = _ptpccenv->new_ins_ord_nord_action(_tid,_xct,_final_rvp,whid);
    ins_ord_nord->postset(did,nextoid,_noin._c_id,_noin._tstamp,olcnt,_noin._all_local);

    irpImpl* my_ord_part = _ptpccenv->ord()->myPart(whid-1);


    // 2b. Insert (NORD)
    ins_nord_nord_action* ins_nord_nord = _ptpccenv->new_ins_nord_nord_action(_tid,_xct,_final_rvp,whid);
    ins_nord_nord->postset(did,nextoid);

    irpImpl* my_nord_part = _ptpccenv->nor()->myPart(whid-1);


    // 5. Enqueue the (Midway->Final) actions
    {
        // ORD_PART_CS
        CRITICAL_SECTION(ord_part_cs, my_ord_part->_enqueue_lock);

        if (my_ord_part->enqueue(ins_ord_nord,_bWake)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing INS_ORD_NORD\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }

        // NORD_PART_CS
        CRITICAL_SECTION(nord_part_cs, my_nord_part->_enqueue_lock);
        ord_part_cs.exit();

        if (my_nord_part->enqueue(ins_nord_nord,_bWake)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing INS_NORD_NORD\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }
    }
    


    // 2c. OL_CNT x Insert (OL)

    for (int i=0;i<olcnt;i++) {
                
        // 8a. Generate the actions
        ins_ol_nord_action* ins_ol_nord = _ptpccenv->new_ins_ol_nord_action(_tid,_xct,_final_rvp,whid);
        ins_ol_nord->postset(did,nextoid,i,_noin.items[i],_noin._tstamp);

        {
            irpImpl* my_ol_part = _ptpccenv->oli()->myPart(whid-1);

            // ITEM_PART_CS
            CRITICAL_SECTION(oli_part_cs, my_ol_part->_enqueue_lock);

            if (my_ol_part->enqueue(ins_ol_nord,_bWake)) {
                TRACE( TRACE_DEBUG, "Problem in enqueueing INS_OL_NORD-%d\n", i);
                assert (0); 
                return (RC(de_PROBLEM_ENQUEUE));
            }
        }
    }

    return (RCOK);
}




/******************************************************************** 
 *
 * NEW_ORDER FINAL RVP
 *
 ********************************************************************/

w_rc_t final_nord_rvp::run() 
{
    return (_run(_ptpccenv)); 
}

void final_nord_rvp::upd_committed_stats() 
{
    _ptpccenv->_inc_new_order_att();
}                     

void final_nord_rvp::upd_aborted_stats() 
{
    _ptpccenv->_inc_new_order_failed();
}                     




/******************************************************************** 
 *
 * NEWORDER TPC-C DORA ACTIONS
 *
 ********************************************************************/


/******************************************************************** 
 *
 * - Start -> Final
 *
 * (1) R_WH_NORD_ACTION
 * (2) R_CUST_NORD_ACTION
 *
 ********************************************************************/


// R_WH_NORD_ACTION

void r_wh_nord_action::calc_keys() 
{
    set_read_only();
    _down.push_back(_wh_id);
    _up.push_back(_wh_id);
}


w_rc_t r_wh_nord_action::trx_exec() 
{
    assert (_ptpccenv);

    // get table tuple from the cache
    row_impl<warehouse_t>* prwh = _ptpccenv->warehouse_man()->get_tuple();
    assert (prwh);


    rep_row_t areprow(_ptpccenv->warehouse_man()->ts());
    areprow.set(_ptpccenv->warehouse_desc()->maxsize()); 
    prwh->_rep = &areprow;

    w_rc_t e = RCOK;

    { // make gotos safe

        /* SELECT w_tax
         * FROM warehouse
         * WHERE w_id = :w_id
         *
         * plan: index probe on "W_INDEX"
         */

        /* 1. retrieve warehouse (read-only) */
        TRACE( TRACE_TRX_FLOW, "App: %d NO:wh-idx-nl (%d)\n", _tid, _wh_id);

#ifndef ONLYDORA
        e = _ptpccenv->warehouse_man()->wh_index_probe_nl(_ptpccenv->db(), 
                                                          prwh, _wh_id);
#endif

        if (e.is_error()) { goto done; }

        tpcc_warehouse_tuple awh;
        prwh->get_value(7, awh.W_TAX);

    } // goto

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prwh->print_tuple();
#endif

done:
    // give back the tuple
    _ptpccenv->warehouse_man()->give_tuple(prwh);
    return (e);
}




// R_CUST_NORD_ACTION

void r_cust_nord_action::calc_keys() 
{
    set_read_only();
    _down.push_back(_wh_id);
    _down.push_back(_d_id);
    _up.push_back(_wh_id);
    _up.push_back(_d_id);
}


w_rc_t r_cust_nord_action::trx_exec() 
{
    assert (_ptpccenv);

    // get table tuple from the cache
    row_impl<customer_t>* prcust = _ptpccenv->customer_man()->get_tuple();
    assert (prcust);

    rep_row_t areprow(_ptpccenv->customer_man()->ts());
    areprow.set(_ptpccenv->customer_desc()->maxsize()); 
    prcust->_rep = &areprow;

    w_rc_t e = RCOK;

    { // make gotos safe

        /* SELECT c_discount, c_last, c_credit
         * FROM customer
         * WHERE w_id = :w_id AND c_d_id = :d_id AND c_id = :c_id
         *
         * plan: index probe on "C_INDEX"
         */


        /* 1. retrieve customer (read-only) */
        TRACE( TRACE_TRX_FLOW, 
               "App: %d NO:cust-idx-nl (%d) (%d) (%d)\n", 
               _tid, _wh_id, _d_id, _c_id);

#ifndef ONLYDORA
        e = _ptpccenv->customer_man()->cust_index_probe_nl(_ptpccenv->db(), prcust,
                                                           _wh_id, _d_id, _c_id);
#endif

        if (e.is_error()) { goto done; }

        tpcc_customer_tuple  acust;
        prcust->get_value(15, acust.C_DISCOUNT);
        prcust->get_value(13, acust.C_CREDIT, 3);
        prcust->get_value(5, acust.C_LAST, 17);

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







/******************************************************************** 
 *
 * - Start -> Midway
 *
 * (3) UPD_DIST_NORD_ACTION
 * (4) R_ITEM_NORD_ACTION
 * (5) UPD_ITEM_NORD_ACTION
 *
 * @note: Those actions need to report something to the next (midway) RVP.
 *        Therefore, at the end of each action there should be some 
 *        update of data on the RVP.  
 *
 ********************************************************************/


// UPD_DIST_NORD_ACTION

void upd_dist_nord_action::calc_keys() 
{
    _down.push_back(_wh_id);
    _down.push_back(_d_id);
    _up.push_back(_wh_id);
    _up.push_back(_d_id);
}

w_rc_t upd_dist_nord_action::trx_exec() 
{
    assert (_ptpccenv);

    // get table tuple from the cache
    row_impl<district_t>* prdist = _ptpccenv->district_man()->get_tuple();
    assert (prdist);

    rep_row_t areprow(_ptpccenv->district_man()->ts());
    areprow.set(_ptpccenv->district_desc()->maxsize()); 
    prdist->_rep = &areprow;

    w_rc_t e = RCOK;

    { // make gotos safe

        /* SELECT d_tax, d_next_o_id
         * FROM district
         * WHERE d_id = :d_id AND d_w_id = :w_id
         *
         * plan: index probe on "D_INDEX"
         */

        /* 1. retrieve district for update */
        TRACE( TRACE_TRX_FLOW, 
               "App: %d NO:dist-idx-nl (%d) (%d)\n", _tid, _wh_id, _d_id);

#ifndef ONLYDORA
        e = _ptpccenv->district_man()->dist_index_probe_nl(_ptpccenv->db(), 
                                                           prdist, 
                                                           _wh_id, _d_id);
#endif

        if (e.is_error()) { goto done; }

        tpcc_district_tuple adist;
        prdist->get_value(8, adist.D_TAX);
        prdist->get_value(10, adist.D_NEXT_O_ID);
        adist.D_NEXT_O_ID++;


        /* UPDATE district
         * SET d_next_o_id = :d_next_o_id+1
         * WHERE CURRENT OF dist_cur
         */


        /* 2. update next_o_id */
        TRACE( TRACE_TRX_FLOW, 
               "App: %d NO:dist-upd-next-o-id-nl (%d)\n", 
               _tid, adist.D_NEXT_O_ID);

#ifndef ONLYDORA
        e = _ptpccenv->district_man()->dist_update_next_o_id_nl(_ptpccenv->db(), 
                                                                prdist, 
                                                                adist.D_NEXT_O_ID);
#endif

        if (e.is_error()) { goto done; }

        // update midway RVP 
        _pmidway_rvp->_noin._d_next_o_id = adist.D_NEXT_O_ID;

    } // goto

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prdist->print_tuple();
#endif

done:
    // give back the tuple
    _ptpccenv->district_man()->give_tuple(prdist);
    return (e);
}



// R_ITEM_NORD_ACTION

void r_item_nord_action::calc_keys() 
{
    set_read_only();
    _down.push_back(_pmidway_rvp->_noin.items[_ol_idx]._ol_i_id);
    _up.push_back(_pmidway_rvp->_noin.items[_ol_idx]._ol_i_id);
}

w_rc_t r_item_nord_action::trx_exec() 
{
    assert (_ptpccenv);

    // get table tuple from the cache
    row_impl<item_t>* pritem = _ptpccenv->item_man()->get_tuple();
    assert (pritem);

    rep_row_t areprow(_ptpccenv->item_man()->ts());
    areprow.set(_ptpccenv->item_desc()->maxsize()); 
    pritem->_rep = &areprow;

    w_rc_t e = RCOK;

    { // make gotos safe

        /* 4. probe item (read-only) */
        register int ol_i_id = _pmidway_rvp->_noin.items[_ol_idx]._ol_i_id;
        register int ol_supply_w_id = _pmidway_rvp->_noin.items[_ol_idx]._ol_supply_wh_id;


        /* SELECT i_price, i_name, i_data
         * FROM item
         * WHERE i_id = :ol_i_id
         *
         * plan: index probe on "I_INDEX"
         */

        TRACE( TRACE_TRX_FLOW, "App: %d NO:item-idx-nl (%d)\n", 
               _tid, ol_i_id);

#ifndef ONLYDORA
        e = _ptpccenv->item_man()->it_index_probe_nl(_ptpccenv->db(), pritem, 
                                                     ol_i_id);
#endif

        if (e.is_error()) { goto done; }


        // update midway RVP 
        tpcc_item_tuple* pitem = &_pmidway_rvp->_noin.items[_ol_idx]._aitem;
        pritem->get_value(4, pitem->I_DATA, 51);
        pritem->get_value(3, pitem->I_PRICE);
        pritem->get_value(2, pitem->I_NAME, 25);

        _pmidway_rvp->_noin.items[_ol_idx]._item_amount = 
            pitem->I_PRICE * _pmidway_rvp->_noin.items[_ol_idx]._ol_quantity; 

    } // goto

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    pritem->print_tuple();
#endif

done:
    // give back the tuple
    _ptpccenv->item_man()->give_tuple(pritem);
    return (e);
}



// UPD_STO_NORD_ACTION

#warning Only 1 field (WH) determines the STOCK table accesses! Not 2.

void upd_sto_nord_action::calc_keys() 
{
    _down.push_back(_pmidway_rvp->_noin.items[_ol_idx]._ol_supply_wh_id);
    _up.push_back(_pmidway_rvp->_noin.items[_ol_idx]._ol_supply_wh_id);
}


w_rc_t upd_sto_nord_action::trx_exec() 
{
    assert (_ptpccenv);

    // get table tuple from the cache
    row_impl<stock_t>* prst = _ptpccenv->stock_man()->get_tuple();
    assert (prst);

    rep_row_t areprow(_ptpccenv->item_man()->ts());
    areprow.set(_ptpccenv->stock_desc()->maxsize()); 
    prst->_rep = &areprow;

    w_rc_t e = RCOK;

    { // make gotos safe

        /* 4. probe stock (for update) */
        register int ol_i_id = _pmidway_rvp->_noin.items[_ol_idx]._ol_i_id;
        register int ol_supply_w_id = _pmidway_rvp->_noin.items[_ol_idx]._ol_supply_wh_id;


        /* SELECT s_quantity, s_remote_cnt, s_data, s_dist0, s_dist1, s_dist2, ...
         * FROM stock
         * WHERE s_i_id = :ol_i_id AND s_w_id = :ol_supply_w_id
         *
         * plan: index probe on "S_INDEX"
         */

        tpcc_stock_tuple* pstock = &_pmidway_rvp->_noin.items[_ol_idx]._astock;
        tpcc_item_tuple*  pitem  = &_pmidway_rvp->_noin.items[_ol_idx]._aitem;
        TRACE( TRACE_TRX_FLOW, "App: %d NO:stock-idx-nl (%d) (%d)\n", 
               _tid, ol_supply_w_id, ol_i_id);

#ifndef ONLYDORA
        e = _ptpccenv->stock_man()->st_index_probe_nl(_ptpccenv->db(), prst, 
                                                      ol_supply_w_id, ol_i_id);
#endif

        if (e.is_error()) { goto done; }

        prst->get_value(0, pstock->S_I_ID);
        prst->get_value(1, pstock->S_W_ID);
        prst->get_value(5, pstock->S_YTD);
        pstock->S_YTD += _pmidway_rvp->_noin.items[_ol_idx]._ol_quantity;
        prst->get_value(2, pstock->S_REMOTE_CNT);        
        prst->get_value(3, pstock->S_QUANTITY);
        pstock->S_QUANTITY -= _pmidway_rvp->_noin.items[_ol_idx]._ol_quantity;
        if (pstock->S_QUANTITY < 10) pstock->S_QUANTITY += 91;
        prst->get_value(6+_d_id, pstock->S_DIST[6+_d_id], 25);
        prst->get_value(16, pstock->S_DATA, 51);

        char c_s_brand_generic;
        if (strstr(pitem->I_DATA, "ORIGINAL") != NULL && 
            strstr(pstock->S_DATA, "ORIGINAL") != NULL)
            c_s_brand_generic = 'B';
        else c_s_brand_generic = 'G';

        prst->get_value(4, pstock->S_ORDER_CNT);
        pstock->S_ORDER_CNT++;

        if (_wh_id != ol_supply_w_id) pstock->S_REMOTE_CNT++;


        /* UPDATE stock
         * SET s_quantity = :s_quantity, s_order_cnt = :s_order_cnt
         * WHERE s_w_id = :w_id AND s_i_id = :ol_i_id;
         */

        TRACE( TRACE_TRX_FLOW, "App: %d NO:stock-upd-tuple-nl (%d) (%d)\n", 
               _tid, pstock->S_W_ID, pstock->S_I_ID);


#ifndef ONLYDORA
        e = _ptpccenv->stock_man()->st_update_tuple_nl(_ptpccenv->db(), prst, 
                                                       pstock);
#endif

        if (e.is_error()) { goto done; }

        // update RVP
        // The RVP is updated throught the pstock

    } // goto

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prst->print_tuple();
#endif

done:
    // give back the tuple
    _ptpccenv->stock_man()->give_tuple(prst);
    return (e);
}





/******************************************************************** 
 *
 * - Midway -> Final
 *
 * (6) INS_ORD_NORD_ACTION
 * (7) INS_NORD_NORD_ACTION
 * (8) INS_OL_NORD_ACTION
 *
 ********************************************************************/


// INS_ORD_NORD_ACTION


#warning Only 2 fields (WH,DI) determine the ORDER table accesses! Not 3.

void ins_ord_nord_action::calc_keys() 
{
    _down.push_back(_wh_id);
    _down.push_back(_d_id);
    _up.push_back(_wh_id);
    _up.push_back(_d_id);
}


w_rc_t ins_ord_nord_action::trx_exec() 
{
    assert (_ptpccenv);

    // get table tuple from the cache
    row_impl<order_t>* prord = _ptpccenv->order_man()->get_tuple();
    assert (prord);


    rep_row_t areprow(_ptpccenv->order_man()->ts());
    areprow.set(_ptpccenv->order_desc()->maxsize()); 
    prord->_rep = &areprow;

    w_rc_t e = RCOK;

    { // make gotos safe

        /* 5. insert row to orders */

        /* INSERT INTO orders
         * VALUES (o_id, o_d_id, o_w_id, o_c_id, o_entry_d, o_ol_cnt, o_all_local)
         */

        prord->set_value(0, _d_next_o_id);
        prord->set_value(1, _c_id);
        prord->set_value(2, _d_id);
        prord->set_value(3, _wh_id);
        prord->set_value(4, _tstamp);
        prord->set_value(5, 0);
        prord->set_value(6, _ol_cnt);
        prord->set_value(7, _all_local);

        TRACE( TRACE_TRX_FLOW, "App: %d NO:ord-add-tuple-nl (%d)\n", 
               _tid, _d_next_o_id);

#ifndef ONLYDORA
        e = _ptpccenv->order_man()->add_tuple(_ptpccenv->db(), prord, NL);
#endif

        if (e.is_error()) { goto done; }

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



// INS_NORD_NORD_ACTION


#warning Only 2 fields (WH,DI) determine the NEW-ORDER table accesses! Not 3.

void ins_nord_nord_action::calc_keys() 
{
    _down.push_back(_wh_id);
    _down.push_back(_d_id);
    _up.push_back(_wh_id);
    _up.push_back(_d_id);
}


w_rc_t ins_nord_nord_action::trx_exec() 
{
    assert (_ptpccenv);

    // get table tuple from the cache
    row_impl<new_order_t>* prno = _ptpccenv->new_order_man()->get_tuple();
    assert (prno);


    rep_row_t areprow(_ptpccenv->new_order_man()->ts());
    areprow.set(_ptpccenv->new_order_desc()->maxsize()); 
    prno->_rep = &areprow;

    w_rc_t e = RCOK;

    { // make gotos safe

        /* 5. insert row to new_order */

        /* INSERT INTO new_order VALUES (o_id, d_id, w_id)
         */

        prno->set_value(0, _d_next_o_id);
        prno->set_value(1, _d_id);
        prno->set_value(2, _wh_id);

        TRACE( TRACE_TRX_FLOW, "App: %d NO:nord-add-tuple (%d) (%d) (%d)\n", 
               _tid, _wh_id, _d_id, _d_next_o_id);
    
#ifndef ONLYDORA
        e = _ptpccenv->new_order_man()->add_tuple(_ptpccenv->db(), prno, NL);
#endif

        if (e.is_error()) { goto done; }

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



// INS_OL_NORD_ACTION


#warning Only 2 fields (WH,DI) determine the ORDERLINE table accesses! Not 3.

void ins_ol_nord_action::calc_keys() 
{
    _down.push_back(_wh_id);
    _down.push_back(_d_id);
    _up.push_back(_wh_id);
    _up.push_back(_d_id);
}


w_rc_t ins_ol_nord_action::trx_exec() 
{
    assert (_ptpccenv);

    // get table tuple from the cache
    row_impl<order_line_t>* prol = _ptpccenv->order_line_man()->get_tuple();
    assert (prol);

    rep_row_t areprow(_ptpccenv->order_line_man()->ts());
    areprow.set(_ptpccenv->order_line_desc()->maxsize()); 
    prol->_rep = &areprow;

    w_rc_t e = RCOK;

    { // make gotos safe

        /* 4. insert row to order_line */

        /* INSERT INTO order_line
         * VALUES (o_id, d_id, w_id, ol_ln, ol_i_id, supply_w_id,
         *        '0001-01-01-00.00.01.000000', ol_quantity, iol_amount, dist)
         */

        prol->set_value(0, _d_next_o_id);
        prol->set_value(1, _d_id);
        prol->set_value(2, _wh_id);
        prol->set_value(3, _ol_idx+1);
        prol->set_value(4, _item_info._ol_i_id);
        prol->set_value(5, _item_info._ol_supply_wh_id);
        prol->set_value(6, _tstamp);
        prol->set_value(7, _item_info._ol_quantity);
        prol->set_value(8, _item_info._item_amount);
        prol->set_value(9, _item_info._astock.S_DIST[6+_d_id]);

        TRACE( TRACE_TRX_FLOW, 
               "App: %d NO:ol-add-tuple (%d) (%d) (%d) (%d)\n", 
               _tid, _wh_id, _d_id, _d_next_o_id, _ol_idx+1);

#ifndef ONLYDORA
        e = _ptpccenv->order_line_man()->add_tuple(_ptpccenv->db(), prol, NL);
#endif

        if (e.is_error()) { goto done; }

    } // goto

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prol->print_tuple();
#endif

done:
    // give back the tuple
    _ptpccenv->order_line_man()->give_tuple(prol);
    return (e);
}





EXIT_NAMESPACE(dora);
