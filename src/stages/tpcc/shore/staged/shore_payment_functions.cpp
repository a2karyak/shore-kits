/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_payment_functions.cpp
 *
 *  @brief Implementation of functionality for SHORE_PAYMENT_STAGED transactions
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#include "workload/tpcc/shore_tpcc_env.h"


using namespace qpipe;
using namespace shore;
using namespace tpcc;


/** Exported Functions */


////////////////////////////////////////////////////////////
// STAGED Functions
////////////////////////////////////////////////////////////



/******************************************************************** 
 *
 *  @fn:     staged_pay_updateShoreWarehouse
 *
 *  @brief:  Updates the Shore Warehouse.
 *
 *  @todo:   Could happen without locking.
 * 
 *  @return: 0 on success, non-zero otherwise
 *
 ********************************************************************/

w_rc_t ShoreTPCCEnv::staged_pay_updateShoreWarehouse(payment_input_t* ppin, 
                                                     const int xct_id, 
                                                     trx_result_tuple_t& trt)
{
    assert (ppin);
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    // get table tuple from the cache
    row_impl<warehouse_t>* prwh = _pwarehouse_man->get_tuple();
    assert (prwh);

    rep_row_t areprow(_pwarehouse_man->ts());
    areprow.set(_pwarehouse_desc->maxsize()); 
    prwh->_rep = &areprow;

    // assume that sub-task will fail
    trt.set_id(xct_id);
    trt.set_state(POISSONED);

    /* 1. retrieve warehouse for update */
    TRACE( TRACE_TRX_FLOW, 
           "App: %d PAY:warehouse-idx-probe (%d)\n", 
           xct_id, ppin->_home_wh_id);

    W_DO(_pwarehouse_man->wh_index_probe_forupdate(_pssm, prwh, ppin->_home_wh_id));   

    /* UPDATE warehouse SET w_ytd = wytd + :h_amount
     * WHERE w_id = :w_id
     *
     * plan: index probe on "W_INDEX"
     */

    TRACE( TRACE_TRX_FLOW, "App: %d PAY:wh-update-ytd2 (%d)\n", 
           xct_id, ppin->_home_wh_id);
    W_DO(_pwarehouse_man->wh_update_ytd(_pssm, prwh, ppin->_h_amount));

    tpcc_warehouse_tuple awh;
    prwh->get_value(1, awh.W_NAME, 11);
    prwh->get_value(2, awh.W_STREET_1, 21);
    prwh->get_value(3, awh.W_STREET_2, 21);
    prwh->get_value(4, awh.W_CITY, 21);
    prwh->get_value(5, awh.W_STATE, 3);
    prwh->get_value(6, awh.W_ZIP, 10);

    // give back the tuple
    _pwarehouse_man->give_tuple(prwh);

    // if we reached this point everything went ok
    trt.set_state(SUBMITTED);

    return (RCOK);
}



/******************************************************************** 
 *
 *  @fn:     staged_pay_updateShoreDistrict
 *
 *  @brief:  Updates the Shore District 
 *
 *  @note:   Normally that could happen without locking.
 * 
 *  @return: 0 on success, non-zero otherwise
 *
 ********************************************************************/

w_rc_t ShoreTPCCEnv::staged_pay_updateShoreDistrict(payment_input_t* ppin, 
                                                    const int xct_id, 
                                                    trx_result_tuple_t& trt)
{
    assert (ppin);
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);
    
    // get table tuple from the caches
    row_impl<district_t>* prdist = _pdistrict_man->get_tuple();
    assert (prdist);

    rep_row_t areprow(_pdistrict_man->ts());
    areprow.set(_pdistrict_desc->maxsize()); 
    prdist->_rep = &areprow;

    // assume that sub-task will fail
    trt.set_id(xct_id);
    trt.set_state(POISSONED);

    /* 2. retrieve district for update */
    TRACE( TRACE_TRX_FLOW, 
           "App: %d PAY:district-idx-probe (%d) (%d)\n", 
           xct_id, ppin->_home_wh_id, ppin->_home_d_id);

    W_DO(_pdistrict_man->dist_index_probe_forupdate(_pssm, prdist,
                                                    ppin->_home_d_id, 
                                                    ppin->_home_wh_id));

    /* UPDATE district SET d_ytd = d_ytd + :h_amount
     * WHERE d_id = :d_id AND d_w_id = :w_id
     *
     * plan: index probe on "D_INDEX"
     */

    TRACE( TRACE_TRX_FLOW, "App: %d PAY:distr-update-ytd1 (%d) (%d)\n", 
           xct_id, ppin->_home_wh_id, ppin->_home_d_id);
    W_DO(_pdistrict_man->dist_update_ytd(_pssm, prdist, ppin->_h_amount));


    /* SELECT d_street_1, d_street_2, d_city, d_state, d_zip, d_name
     * FROM district
     * WHERE d_id = :d_id AND d_w_id = :w_id
     *
     * SELECT d_street_1, d_street_2, d_city, d_state, d_zip, d_name
     * FROM district
     * WHERE d_id = :d_id AND d_w_id = :w_id
     *
     * plan: index probe on "D_INDEX"
     */

    tpcc_district_tuple adistr;
    prdist->get_value(2, adistr.D_NAME, 11);
    prdist->get_value(3, adistr.D_STREET_1, 21);
    prdist->get_value(4, adistr.D_STREET_2, 21);
    prdist->get_value(5, adistr.D_CITY, 21);
    prdist->get_value(6, adistr.D_STATE, 3);
    prdist->get_value(7, adistr.D_ZIP, 10);

    // give back the tuple
    _pdistrict_man->give_tuple(prdist);

    // if we reached this point everything went ok
    trt.set_state(SUBMITTED);

    return (RCOK);
}



/******************************************************************** 
 *
 *  @fn:     staged_pay_updateShoreDistrict
 *
 *  @brief:  Updates the Shore District 
 *
 *  @note:   Normally that could happen without locking.
 * 
 *  @return: 0 on success, non-zero otherwise
 *
 ********************************************************************/

w_rc_t ShoreTPCCEnv::staged_pay_updateShoreCustomer(payment_input_t* ppin, 
                                                    const int xct_id, 
                                                    trx_result_tuple_t& trt)
{
    assert (ppin);
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    // get table tuple from the caches
    row_impl<customer_t>* prcust = _pcustomer_man->get_tuple();
    assert (prcust);

    rep_row_t areprow(_pcustomer_man->ts());
    areprow.set(_pcustomer_desc->maxsize()); 
    prcust->_rep = &areprow;

    // assume that sub-task will fail
    trt.set_id(xct_id);
    trt.set_state(POISSONED);

    // find the customer wh and d
    int c_w = ( ppin->_v_cust_wh_selection > 85 ? ppin->_home_wh_id : ppin->_remote_wh_id);
    int c_d = ( ppin->_v_cust_wh_selection > 85 ? ppin->_home_d_id : ppin->_remote_d_id);

    if (ppin->_c_id == 0) {

        /* 3a. if no customer selected already use the index on the customer name */

        /* SELECT  c_id, c_first
         * FROM customer
         * WHERE c_last = :c_last AND c_w_id = :c_w_id AND c_d_id = :c_d_id
         * ORDER BY c_first
         *
         * plan: index only scan on "C_NAME_INDEX"
         */

        assert (ppin->_v_cust_ident_selection <= 60);

        rep_row_t lowrep(_pcustomer_man->ts());
        rep_row_t highrep(_pcustomer_man->ts());

        index_scan_iter_impl<customer_t>* c_iter;
        TRACE( TRACE_TRX_FLOW, "App: %d PAY:cust-get-iter-by-name-index (%s)\n", 
               xct_id, ppin->_c_last);
        W_DO(_pcustomer_man->cust_get_iter_by_index(_pssm, c_iter, prcust, 
                                                    lowrep, highrep,
                                                    c_w, c_d, ppin->_c_last));

        int c_id_list[17];
        int count = 0;
        bool eof;

        W_DO(c_iter->next(_pssm, eof, *prcust));
        while (!eof) {
            prcust->get_value(0, c_id_list[count++]);            
            TRACE( TRACE_TRX_FLOW, "App: %d PAY:cust-iter-next (%d)\n", 
                   xct_id, c_id_list[count]);
            W_DO(c_iter->next(_pssm, eof, *prcust));
        }
        delete c_iter;
        assert (count);

        /* find the customer id in the middle of the list */
        ppin->_c_id = c_id_list[(count+1)/2-1];
    }
    assert (ppin->_c_id>0);


    /* SELECT c_first, c_middle, c_last, c_street_1, c_street_2, c_city, 
     * c_state, c_zip, c_phone, c_since, c_credit, c_credit_lim, 
     * c_discount, c_balance, c_ytd_payment, c_payment_cnt 
     * FROM customer 
     * WHERE c_id = :c_id AND c_w_id = :c_w_id AND c_d_id = :c_d_id 
     * FOR UPDATE OF c_balance, c_ytd_payment, c_payment_cnt
     *
     * plan: index probe on "C_INDEX"
     */

    TRACE( TRACE_TRX_FLOW, 
           "App: %d PAY:cust-idx-probe-forupdate (%d) (%d) (%d)\n", 
           xct_id, c_w, c_d, ppin->_c_id);

    W_DO(_pcustomer_man->cust_index_probe_forupdate(_pssm, prcust, 
                                                    ppin->_c_id, c_w, c_d));

    double c_balance, c_ytd_payment;
    int    c_payment_cnt;
    tpcc_customer_tuple acust;

    // retrieve customer
    prcust->get_value(3,  acust.C_FIRST, 17);
    prcust->get_value(4,  acust.C_MIDDLE, 3);
    prcust->get_value(5,  acust.C_LAST, 17);
    prcust->get_value(6,  acust.C_STREET_1, 21);
    prcust->get_value(7,  acust.C_STREET_2, 21);
    prcust->get_value(8,  acust.C_CITY, 21);
    prcust->get_value(9,  acust.C_STATE, 3);
    prcust->get_value(10, acust.C_ZIP, 10);
    prcust->get_value(11, acust.C_PHONE, 17);
    prcust->get_value(12, acust.C_SINCE);
    prcust->get_value(13, acust.C_CREDIT, 3);
    prcust->get_value(14, acust.C_CREDIT_LIM);
    prcust->get_value(15, acust.C_DISCOUNT);
    prcust->get_value(16, acust.C_BALANCE);
    prcust->get_value(17, acust.C_YTD_PAYMENT);
    prcust->get_value(18, acust.C_LAST_PAYMENT);
    prcust->get_value(19, acust.C_PAYMENT_CNT);
    prcust->get_value(20, acust.C_DATA_1, 251);
    prcust->get_value(21, acust.C_DATA_2, 251);

    // update customer fields
    acust.C_BALANCE -= ppin->_h_amount;
    acust.C_YTD_PAYMENT += ppin->_h_amount;
    acust.C_PAYMENT_CNT++;

    // if bad customer
    if (acust.C_CREDIT[0] == 'B' && acust.C_CREDIT[1] == 'C') { 
        /* 10% of customers */

        /* SELECT c_data
         * FROM customer 
         * WHERE c_id = :c_id AND c_w_id = :c_w_id AND c_d_id = :c_d_id
         * FOR UPDATE OF c_balance, c_ytd_payment, c_payment_cnt, c_data
         *
         * plan: index probe on "C_INDEX"
         */

        // update the data
        char c_new_data_1[251];
        char c_new_data_2[251];
        sprintf(c_new_data_1, "%d,%d,%d,%d,%d,%1.2f",
                ppin->_c_id, c_d, c_w, ppin->_home_d_id, 
                ppin->_home_wh_id, ppin->_h_amount);

        int len = strlen(c_new_data_1);
        strncat(c_new_data_1, acust.C_DATA_1, 250-len);
        strncpy(c_new_data_2, &acust.C_DATA_1[250-len], len);
        strncpy(c_new_data_2, acust.C_DATA_2, 250-len);

        TRACE( TRACE_TRX_FLOW, "App: %d PAY:cust-update-tuple\n", xct_id);
        W_DO(_pcustomer_man->cust_update_tuple(_pssm, prcust, acust, c_new_data_1, c_new_data_2));
    }
    else { /* good customer */
        TRACE( TRACE_TRX_FLOW, "App: %d PAY:cust-update-tuple\n", xct_id);
        W_DO(_pcustomer_man->cust_update_tuple(_pssm, prcust, acust, NULL, NULL));
    }

    // give back the tuple
    _pcustomer_man->give_tuple(prcust);

    // if we reached this point everything went ok
    trt.set_state(SUBMITTED);

    return (RCOK);
}


/******************************************************************** 
 *
 *  @fn:     staged_pay_insertShoreHistory
 *
 *  @brief:  Inserts into Shore History.
 *
 *  @note:   Normally that could happen without locking.
 * 
 *  @return: 0 on success, non-zero otherwise
 *
 ********************************************************************/

w_rc_t ShoreTPCCEnv::staged_pay_insertShoreHistory(payment_input_t* ppin, 
                                                   char* p_wh_name,
                                                   char* p_d_name,
                                                   const int xct_id,
                                                   trx_result_tuple_t& trt)
{
    assert (ppin);
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    // get table tuple from the caches
    row_impl<history_t>* prhist = _phistory_man->get_tuple();
    assert (prhist);

    rep_row_t areprow(_phistory_man->ts());
    areprow.set(_phistory_desc->maxsize()); 
    prhist->_rep = &areprow;

    // assume that sub-task will fail
    trt.set_id(xct_id);
    trt.set_state(POISSONED);

    // find the customer wh and d
    int c_w = ( ppin->_v_cust_wh_selection > 85 ? ppin->_home_wh_id : ppin->_remote_wh_id);
    int c_d = ( ppin->_v_cust_wh_selection > 85 ? ppin->_home_d_id : ppin->_remote_d_id);

    /* INSERT INTO history
     * VALUES (:c_id, :c_d_id, :c_w_id, :d_id, :w_id, 
     *         :curr_tmstmp, :ih_amount, :h_data)
     */

    tpcc_history_tuple ahist;
    //    sprintf(ahist.H_DATA, "%s   %s", awh.W_NAME, adistr.D_NAME);
    sprintf(ahist.H_DATA, "%s   %s", p_wh_name, p_d_name);
    ahist.H_DATE = time(NULL);

    prhist->set_value(0, ppin->_c_id);
    prhist->set_value(1, c_d);
    prhist->set_value(2, c_w);
    prhist->set_value(3, ppin->_home_d_id);
    prhist->set_value(4, ppin->_home_wh_id);
    prhist->set_value(5, ahist.H_DATE);
    prhist->set_value(6, ppin->_h_amount * 100.0);
    prhist->set_value(7, ahist.H_DATA);

    TRACE( TRACE_TRX_FLOW, "App: %d PAY:hist-add-tuple\n", xct_id);
    W_DO(_phistory_man->add_tuple(_pssm, prhist));

    // give back the tuple
    _phistory_man->give_tuple(prhist);

    // if we reached this point everything went ok
    trt.set_state(SUBMITTED);

    return (RCOK);
}


