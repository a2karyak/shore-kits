/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_mbench.cpp
 *
 *  @brief:  DORA MBENCHES
 *
 *  @note:   Implementation of RVPs and Actions that synthesize 
 *           the mbenches according to DORA
 *
 *  @author: Ippokratis Pandis, Oct 2008
 */

#include "dora/tpcc/dora_mbench.h"


using namespace dora;
using namespace shore;
using namespace tpcc;



ENTER_NAMESPACE(dora);


/******************************************************************** 
 *
 * MBench TRXs
 *
 ********************************************************************/

void final_mb_rvp::upd_committed_stats() 
{
    assert (_ptpccenv);
    if (_ptpccenv->get_measure() != MST_MEASURE) {
        return;
    }
    _ptpccenv->get_total_tpcc_stats()->inc_other_com();
    _ptpccenv->get_session_tpcc_stats()->inc_other_com();
    _ptpccenv->get_env_stats()->inc_trx_com();
}                     

void final_mb_rvp::upd_aborted_stats() 
{
    assert (_ptpccenv);
    if (_ptpccenv->get_measure() != MST_MEASURE) {
        return;
    }
    _ptpccenv->get_total_tpcc_stats()->inc_other_att();
    _ptpccenv->get_session_tpcc_stats()->inc_other_att();
    _ptpccenv->get_env_stats()->inc_trx_att();
}                     


/******************************************************************** 
 *
 * MBench WH
 *
 * (1) UPDATE-WAREHOUSE
 *
 ********************************************************************/

w_rc_t upd_wh_mb_action_impl::trx_exec() 
{
    assert (_ptpccenv);

    // generate the input
    double amount = (double)URand(1,1000);

    // mbench trx touches 1 table: 
    // warehouse

    // get table tuples from the caches
    row_impl<warehouse_t>* prwh = _ptpccenv->warehouse_man()->get_tuple();
    assert (prwh);
    rep_row_t areprow(_ptpccenv->warehouse_man()->ts());
    areprow.set(_ptpccenv->warehouse()->maxsize()); 
    prwh->_rep = &areprow;

    /* 1. retrieve warehouse for update */
    TRACE( TRACE_TRX_FLOW, 
           "App: %d PAY:warehouse-idx-probe (%d)\n", _tid, _whid);
    W_DO(_ptpccenv->warehouse_man()->wh_index_probe_nl(_ptpccenv->db(), prwh, 
                                                       _whid));

    /* UPDATE warehouse SET w_ytd = wytd + :h_amount
     * WHERE w_id = :w_id
     *
     * SELECT w_name, w_street_1, w_street_2, w_city, w_state, w_zip
     * FROM warehouse
     * WHERE w_id = :w_id
     *
     * plan: index probe on "W_INDEX"
     */

    TRACE( TRACE_TRX_FLOW, "App: %d PAY:wh-update-ytd (%d)\n", _tid, _whid);
    W_DO(_ptpccenv->warehouse_man()->wh_update_ytd(_ptpccenv->db(), 
                                                   prwh, 
                                                   amount));

    tpcc_warehouse_tuple awh;
    prwh->get_value(1, awh.W_NAME, 11);
    prwh->get_value(2, awh.W_STREET_1, 21);
    prwh->get_value(3, awh.W_STREET_2, 21);
    prwh->get_value(4, awh.W_CITY, 21);
    prwh->get_value(5, awh.W_STATE, 3);
    prwh->get_value(6, awh.W_ZIP, 10);

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prwh->print_tuple();
#endif

    // give back the tuples
    _ptpccenv->warehouse_man()->give_tuple(prwh);
    return (RCOK);
}


/******************************************************************** 
 *
 * MBench CUST
 *
 * (1) UPDATE-CUSTOMER
 *
 ********************************************************************/

w_rc_t upd_cust_mb_action_impl::trx_exec() 
{
    assert (_ptpccenv);

    // generates the input
    double amount = (double)URand(1,1000);

    // mbench trx touches 1 table: customer

    // get table tuple from the cache
    row_impl<customer_t>* prcust = _ptpccenv->customer_man()->get_tuple();
    assert (prcust);
    rep_row_t areprow(_ptpccenv->customer_man()->ts());
    areprow.set(_ptpccenv->customer()->maxsize()); 
    prcust->_rep = &areprow;

    /* 1. retrieve customer for update */

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
           _tid, _whid, _did, _cid);

    W_DO(_ptpccenv->customer_man()->cust_index_probe_nl(_ptpccenv->db(), prcust, 
                                                        _cid, _whid, _did));
    
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
    acust.C_BALANCE -= amount;
    acust.C_YTD_PAYMENT += amount;
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
                _cid, _did, _whid, _did, _whid, amount);

        int len = strlen(c_new_data_1);
        strncat(c_new_data_1, acust.C_DATA_1, 250-len);
        strncpy(c_new_data_2, &acust.C_DATA_1[250-len], len);
        strncpy(c_new_data_2, acust.C_DATA_2, 250-len);

        TRACE( TRACE_TRX_FLOW, "App: %d PAY:bad-cust-update-tuple\n", _tid);
        W_DO(_ptpccenv->customer_man()->cust_update_tuple(_ptpccenv->db(), 
                                                          prcust, acust, 
                                                          c_new_data_1, 
                                                          c_new_data_2));
    }
    else { /* good customer */
        TRACE( TRACE_TRX_FLOW, "App: %d PAY:good-cust-update-tuple\n", _tid);
        W_DO(_ptpccenv->customer_man()->cust_update_tuple(_ptpccenv->db(), 
                                                          prcust, 
                                                          acust, 
                                                          NULL, 
                                                          NULL));
    }

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prcust->print_tuple();
#endif

    // give back the tuple
    _ptpccenv->customer_man()->give_tuple(prcust);
    return (RCOK);
}


EXIT_NAMESPACE(dora);
