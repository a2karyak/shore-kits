/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_new_order_functions.cpp
 *
 *  @brief Implementation of functionality for SHORE_NEW_ORDER_STAGED transactions
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#include "stages/tpcc/shore/shore_tpcc_env.h"


using namespace qpipe;
using namespace shore;
using namespace tpcc;


/** Exported Functions */


////////////////////////////////////////////////////////////
// STAGED Functions
////////////////////////////////////////////////////////////



/******************************************************************** 
 *
 *  @fn:     staged_no_outside_loop
 *
 *  @brief:  Does the NewOrder operations outside the loop
 * 
 *  @return: 0 on success, non-zero otherwise
 *
 ********************************************************************/

w_rc_t ShoreTPCCEnv::staged_no_outside_loop(new_order_input_t* pnoin, 
                                            time_t tstamp,
                                            const int xct_id, 
                                            trx_result_tuple_t& trt)
{
    assert (pnoin);
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    // warehouse, district, customer, neworder, order    
    row_impl<warehouse_t>* prwh = _pwarehouse_man->get_tuple();
    assert (prwh);
    row_impl<district_t>* prdist = _pdistrict_man->get_tuple();
    assert (prdist);
    row_impl<customer_t>* prcust = _pcustomer_man->get_tuple();
    assert (prcust);
    row_impl<new_order_t>* prno = _pnew_order_man->get_tuple();
    assert (prno);
    row_impl<order_t>* prord = _porder_man->get_tuple();
    assert (prord);

    rep_row_t areprow(_pcustomer_man->ts());
    areprow.set(_pcustomer_desc->maxsize()); 
    prwh->_rep = &areprow;
    prdist->_rep = &areprow;
    prcust->_rep = &areprow;
    prno->_rep = &areprow;
    prord->_rep = &areprow;

    // assume that sub-task will fail
    trt.set_id(xct_id);
    trt.set_state(POISSONED);

    /* SELECT c_discount, c_last, c_credit, w_tax
     * FROM customer, warehouse
     * WHERE w_id = :w_id AND c_w_id = w_id AND c_d_id = :d_id AND c_id = :c_id
     *
     * plan: index probe on "W_INDEX", index probe on "C_INDEX"
     */

    /* 1. retrieve warehouse for update */
    TRACE( TRACE_TRX_FLOW, 
           "App: %d NO:warehouse-idx-probe (%d)\n", 
           xct_id, pnoin->_wh_id);
    W_DO(_pwarehouse_man->wh_index_probe(_pssm, prwh, pnoin->_wh_id));

    tpcc_warehouse_tuple awh;
    prwh->get_value(7, awh.W_TAX);


    /* 2. retrieve district for update */
    TRACE( TRACE_TRX_FLOW, 
           "App: %d NO:district-idx-probe (%d) (%d)\n", 
           xct_id, pnoin->_wh_id, pnoin->_d_id);
    W_DO(_pdistrict_man->dist_index_probe_forupdate(_pssm, prdist, 
                                                    pnoin->_d_id, pnoin->_wh_id));

    /* SELECT d_tax, d_next_o_id
     * FROM district
     * WHERE d_id = :d_id AND d_w_id = :w_id
     *
     * plan: index probe on "D_INDEX"
     */

    tpcc_district_tuple adist;
    prdist->get_value(8, adist.D_TAX);
    prdist->get_value(10, adist.D_NEXT_O_ID);
    adist.D_NEXT_O_ID++;


    /* 3. retrieve customer */
    TRACE( TRACE_TRX_FLOW, 
           "App: %d NO:customer-idx-probe (%d) (%d) (%d)\n", 
           xct_id, pnoin->_wh_id, pnoin->_d_id, pnoin->_c_id);
    W_DO(_pcustomer_man->cust_index_probe(_pssm, prcust, pnoin->_c_id, 
                                          pnoin->_wh_id, pnoin->_d_id));

    tpcc_customer_tuple  acust;
    prcust->get_value(15, acust.C_DISCOUNT);
    prcust->get_value(13, acust.C_CREDIT, 3);
    prcust->get_value(5, acust.C_LAST, 17);

    /* UPDATE district
     * SET d_next_o_id = :next_o_id+1
     * WHERE CURRENT OF dist_cur
     */

    TRACE( TRACE_TRX_FLOW, "App: %d NO:district-upd-next-o-id\n", xct_id);
    W_DO(_pdistrict_man->dist_update_next_o_id(_pssm, prdist, adist.D_NEXT_O_ID));


    /* 5. insert row to orders and new_order */

    /* INSERT INTO orders
     * VALUES (o_id, o_d_id, o_w_id, o_c_id, o_entry_d, o_ol_cnt, o_all_local)
     */

    prord->set_value(0, adist.D_NEXT_O_ID);
    prord->set_value(1, pnoin->_c_id);
    prord->set_value(2, pnoin->_d_id);
    prord->set_value(3, pnoin->_wh_id);
    prord->set_value(4, tstamp);
    prord->set_value(5, 0);
    prord->set_value(6, pnoin->_ol_cnt);
    assert (false);
    //    prord->set_value(7, all_local);

    TRACE( TRACE_TRX_FLOW, "App: %d NO:order-add-tuple (%d)\n", 
           xct_id, adist.D_NEXT_O_ID);
    W_DO(_porder_man->add_tuple(_pssm, prord));


    /* INSERT INTO new_order VALUES (o_id, d_id, w_id)
     */

    prno->set_value(0, adist.D_NEXT_O_ID);
    prno->set_value(1, pnoin->_d_id);
    prno->set_value(2, pnoin->_wh_id);

    TRACE( TRACE_TRX_FLOW, "App: %d NO:new-order-add-tuple (%d) (%d) (%d)\n", 
           xct_id, pnoin->_wh_id, pnoin->_d_id, adist.D_NEXT_O_ID);
    W_DO(_pnew_order_man->add_tuple(_pssm, prno));

    // give back the tuples
    _pwarehouse_man->give_tuple(prwh);
    _pdistrict_man->give_tuple(prdist);
    _pcustomer_man->give_tuple(prcust);
    _pnew_order_man->give_tuple(prno);
    _porder_man->give_tuple(prord);

    // if we reached this point everything went ok
    trt.set_state(SUBMITTED);

    return (RCOK);
}



/******************************************************************** 
 *
 *  @fn:     staged_no_one_ol
 *
 *  @brief:  Does the operations needed for one OL (one iteration of
 *           the loop in) NewOrder 
 * 
 *  @return: 0 on success, non-zero otherwise
 *
 ********************************************************************/

w_rc_t ShoreTPCCEnv::staged_no_one_ol(ol_item_info* polin, 
                                      time_t tstamp,
                                      int a_wh_id,
                                      int a_d_id,
                                      int item_cnt,
                                      const int xct_id, 
                                      trx_result_tuple_t& trt)
{
    assert (polin);
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    // item, stock, orderline
    row_impl<item_t>* pritem = _pitem_man->get_tuple();
    assert (pritem);
    row_impl<stock_t>* prst = _pstock_man->get_tuple();
    assert (prst);
    row_impl<order_line_t>* prol = _porder_line_man->get_tuple();
    assert (prol);
    
    // allocate space for the biggest of the 8 table representations
    rep_row_t areprow(_porder_line_man->ts());
    areprow.set(_porder_line_desc->maxsize()); 
    pritem->_rep = &areprow;
    prst->_rep = &areprow;
    prol->_rep = &areprow;

    // assume that sub-task will fail
    trt.set_id(xct_id);
    trt.set_state(POISSONED);    


    /* 4. for all items update item, stock, and order line */
    register int ol_i_id = polin->_ol_i_id;
    register int ol_supply_w_id = polin->_ol_supply_wh_id;


    /* SELECT i_price, i_name, i_data
     * FROM item
     * WHERE i_id = :ol_i_id
     *
     * plan: index probe on "I_INDEX"
     */

    tpcc_item_tuple aitem;
    TRACE( TRACE_TRX_FLOW, "App: %d NO:item-idx-probe (%d)\n", 
           xct_id, ol_i_id);
    W_DO(_pitem_man->it_index_probe(_pssm, pritem, ol_i_id));

    pritem->get_value(4, aitem.I_DATA, 51);
    pritem->get_value(3, aitem.I_PRICE);
    pritem->get_value(2, aitem.I_NAME, 25);

    int item_amount = aitem.I_PRICE * polin->_ol_quantity; 

    /* SELECT s_quantity, s_remote_cnt, s_data, s_dist0, s_dist1, s_dist2, ...
     * FROM stock
     * WHERE s_i_id = :ol_i_id AND s_w_id = :ol_supply_w_id
     *
     * plan: index probe on "S_INDEX"
     */

    tpcc_stock_tuple astock;
    TRACE( TRACE_TRX_FLOW, "App: %d NO:stock-idx-probe (%d) (%d)\n", 
           xct_id, ol_i_id, ol_supply_w_id);
    W_DO(_pstock_man->st_index_probe_forupdate(_pssm, prst, 
                                               ol_i_id, ol_supply_w_id));

    prst->get_value(0, astock.S_I_ID);
    prst->get_value(1, astock.S_W_ID);
    prst->get_value(5, astock.S_YTD);
    astock.S_YTD += polin->_ol_quantity;
    prst->get_value(2, astock.S_REMOTE_CNT);        
    prst->get_value(3, astock.S_QUANTITY);
    astock.S_QUANTITY -= polin->_ol_quantity;
    if (astock.S_QUANTITY < 10) astock.S_QUANTITY += 91;
    prst->get_value(6+a_d_id, astock.S_DIST[a_d_id], 25);
    prst->get_value(16, astock.S_DATA, 51);

    char c_s_brand_generic;
    if (strstr(aitem.I_DATA, "ORIGINAL") != NULL && 
        strstr(astock.S_DATA, "ORIGINAL") != NULL)
        c_s_brand_generic = 'B';
    else c_s_brand_generic = 'G';

    prst->get_value(4, astock.S_ORDER_CNT);
    astock.S_ORDER_CNT++;

    if (a_wh_id != ol_supply_w_id) {
        astock.S_REMOTE_CNT++;
    }


    /* UPDATE stock
     * SET s_quantity = :s_quantity, s_order_cnt = :s_order_cnt
     * WHERE s_w_id = :w_id AND s_i_id = :ol_i_id;
     */

    TRACE( TRACE_TRX_FLOW, "App: %d NO:stock-upd-tuple (%d) (%d)\n", 
           xct_id, astock.S_W_ID, astock.S_I_ID);
    W_DO(_pstock_man->st_update_tuple(_pssm, prst, &astock));


    /* INSERT INTO order_line
     * VALUES (o_id, d_id, w_id, ol_ln, ol_i_id, supply_w_id,
     *        '0001-01-01-00.00.01.000000', ol_quantity, iol_amount, dist)
     */

    assert (false);
    //prol->set_value(0, adist.D_NEXT_O_ID);
    prol->set_value(1, a_d_id);
    prol->set_value(2, a_wh_id);
    prol->set_value(3, item_cnt+1);
    prol->set_value(4, ol_i_id);
    prol->set_value(5, ol_supply_w_id);
    prol->set_value(6, tstamp);
    prol->set_value(7, polin->_ol_quantity);
    prol->set_value(8, item_amount);
    prol->set_value(9, astock.S_DIST[6+a_d_id]);

    TRACE( TRACE_TRX_FLOW, "App: %d NO:orderline-add-tuple (%d)\n", 
           xct_id, a_d_id);
//     TRACE( TRACE_TRX_FLOW, "App: %d NO:orderline-add-tuple (%d)\n", 
//            xct_id, adist.D_NEXT_O_ID);
    W_DO(_porder_line_man->add_tuple(_pssm, prol));


    // give back the tuples
    _pitem_man->give_tuple(pritem);
    _pstock_man->give_tuple(prst);
    _porder_line_man->give_tuple(prol);

    // if we reached this point everything went ok
    trt.set_state(SUBMITTED);

    return (RCOK);
}


