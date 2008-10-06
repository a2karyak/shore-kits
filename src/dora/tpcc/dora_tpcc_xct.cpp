/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_tpcc_xct.cpp
 *
 *  @brief:  Declaration of the Shore DORA transactions
 *
 *  @author: Ippokratis Pandis, Sept 2008
 */

#include "stages/tpcc/shore/shore_tpcc_env.h"
#include "dora/tpcc/dora_tpcc.h"

using namespace shore;
using namespace dora;
using namespace tpcc;


ENTER_NAMESPACE(dora);


/** DORA TPC-C actions */

/******************************************************************** 
 *
 * TPC-C DORA ACTIONS
 *
 * (1) XXX_action_impl contains 
 *     - The pointer to the TPC-C Shore environment 
 *     - The XXX transaction input
 *     - Code to updates the statistics
 *
 * (2) The YYY_XXX_action_impl contain the implementation of the transactions (trx_exec())
 *
 ********************************************************************/


/** PAYMENT MIDWAY RVP **/

w_rc_t midway_pay_rvp::run() 
{
    // 1. Setup the next RVP
    assert (_result);
    assert (_xct);

    final_pay_rvp* frvp = new final_pay_rvp(_tid, _xct, _result, _ptpccenv);
    assert (frvp);

    // 2. Generate and enqueue action
    ins_hist_pay_action_impl* p_ins_hist_pay_action = _g_dora->get_ins_hist_pay_action();
    //    TRACE( TRACE_DEBUG, "(%d)\n", _tid);


    p_ins_hist_pay_action->set_input(_tid, _xct, frvp, _ptpccenv, _pin);
    assert (p_ins_hist_pay_action);
    frvp->set_pih(p_ins_hist_pay_action);
    p_ins_hist_pay_action->_awh=_awh;
    p_ins_hist_pay_action->_adist=_adist;
    if (_g_dora->his()->enqueue(p_ins_hist_pay_action, 0)) { // One HISTORY partition
            TRACE( TRACE_DEBUG, "Problem in enqueueing INS_HIST_PAY\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
    }

    // 1. Cleanup
    cleanup();

    return (RCOK);
}

void midway_pay_rvp::cleanup() 
{
    TRACE( TRACE_DEBUG, "Cleaning...\n");

    // clean house
    _g_dora->give_action(_puw);
    _g_dora->give_action(_pud);
    _g_dora->give_action(_puc);
}



/** PAYMENT FINAL RVP **/

void final_pay_rvp::upd_committed_stats() 
{
    assert (_ptpccenv);
    if (_ptpccenv->get_measure() == MST_MEASURE) {
        _ptpccenv->get_total_tpcc_stats()->inc_pay_com();
        _ptpccenv->get_session_tpcc_stats()->inc_pay_com();
        _ptpccenv->get_env_stats()->inc_trx_com();
    }
}                     

void final_pay_rvp::upd_aborted_stats() 
{
    assert (_ptpccenv);
    if (_ptpccenv->get_measure() == MST_MEASURE) {
        _ptpccenv->get_total_tpcc_stats()->inc_pay_att();
        _ptpccenv->get_session_tpcc_stats()->inc_pay_att();
        _ptpccenv->get_env_stats()->inc_trx_att();
    }
}                     

void final_pay_rvp::cleanup() 
{
    // clean house
    _g_dora->give_action(_pih);
}



/******************************************************************** 
 *
 * PAYMENT TPC-C DORA ACTIONS
 *
 * (1) UPDATE-WAREHOUSE
 * (2) UPDATE-DISTRICT
 * (3) UPDATE-CUSTOMER
 * (4) INSERT-HISTORY
 *
 ********************************************************************/

w_rc_t upd_wh_pay_action_impl::trx_exec() 
{
    assert (_ptpccenv);
    TRACE( TRACE_TRX_FLOW, "upd warehouse\n");
    _pin.describe(0);

    // get table tuple from the cache
    row_impl<warehouse_t>* prwh = _ptpccenv->warehouse_man()->get_tuple();
    assert (prwh);
    rep_row_t areprow(_ptpccenv->warehouse_man()->ts());
    areprow.set(_ptpccenv->warehouse()->maxsize()); 
    prwh->_rep = &areprow;

    /* 1. retrieve warehouse for update */
    TRACE( TRACE_TRX_FLOW, 
           "App: %d PAY:warehouse-idx-probe (%d)\n", 
           _tid, _pin._home_wh_id);

    W_DO(_ptpccenv->warehouse_man()->wh_index_probe_nl(_ptpccenv->db(), 
                                                       prwh, 
                                                       _pin._home_wh_id));      

    /* UPDATE warehouse SET w_ytd = wytd + :h_amount
     * WHERE w_id = :w_id
     *
     * SELECT w_name, w_street_1, w_street_2, w_city, w_state, w_zip
     * FROM warehouse
     * WHERE w_id = :w_id
     *
     * plan: index probe on "W_INDEX"
     */

    TRACE( TRACE_TRX_FLOW, "App: %d PAY:wh-update-ytd (%d)\n", 
           _tid, _pin._home_wh_id);
    W_DO(_ptpccenv->warehouse_man()->wh_update_ytd(_ptpccenv->db(), 
                                                   prwh, 
                                                   _pin._h_amount));

    tpcc_warehouse_tuple* awh = _m_rvp->wh();
    prwh->get_value(1, awh->W_NAME, 11);
    prwh->get_value(2, awh->W_STREET_1, 21);
    prwh->get_value(3, awh->W_STREET_2, 21);
    prwh->get_value(4, awh->W_CITY, 21);
    prwh->get_value(5, awh->W_STATE, 3);
    prwh->get_value(6, awh->W_ZIP, 10);

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prwh->print_tuple();
#endif

    // give back the tuple
    _ptpccenv->warehouse_man()->give_tuple(prwh);
    return (RCOK);
}

w_rc_t upd_dist_pay_action_impl::trx_exec() 
{
    assert (_ptpccenv);
    TRACE( TRACE_TRX_FLOW, "upd district\n");
    _pin.describe(0);

    // get table tuple from the cache
    row_impl<district_t>* prdist = _ptpccenv->district_man()->get_tuple();
    assert (prdist);
    rep_row_t areprow(_ptpccenv->district_man()->ts());
    areprow.set(_ptpccenv->district()->maxsize()); 
    prdist->_rep = &areprow;

    /* 1. retrieve district for update */
    TRACE( TRACE_TRX_FLOW, 
           "App: %d PAY:district-idx-probe (%d) (%d)\n", 
           _tid, _pin._home_wh_id, _pin._home_d_id);

    W_DO(_ptpccenv->district_man()->dist_index_probe_nl(_ptpccenv->db(), 
                                                        prdist,
                                                        _pin._home_d_id, 
                                                        _pin._home_wh_id));    

    /* UPDATE district SET d_ytd = d_ytd + :h_amount
     * WHERE d_id = :d_id AND d_w_id = :w_id
     *
     * SELECT d_street_1, d_street_2, d_city, d_state, d_zip, d_name
     * FROM district
     * WHERE d_id = :d_id AND d_w_id = :w_id
     *
     * plan: index probe on "D_INDEX"
     */

    TRACE( TRACE_TRX_FLOW, "App: %d PAY:distr-upd-ytd (%d) (%d)\n", 
           _tid, _pin._home_wh_id, _pin._home_d_id);
    W_DO(_ptpccenv->district_man()->dist_update_ytd(_ptpccenv->db(), 
                                                    prdist, 
                                                    _pin._h_amount));

    tpcc_district_tuple* adistr = _m_rvp->dist();
    prdist->get_value(2, adistr->D_NAME, 11);
    prdist->get_value(3, adistr->D_STREET_1, 21);
    prdist->get_value(4, adistr->D_STREET_2, 21);
    prdist->get_value(5, adistr->D_CITY, 21);
    prdist->get_value(6, adistr->D_STATE, 3);
    prdist->get_value(7, adistr->D_ZIP, 10);

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prdist->print_tuple();
#endif

    // give back the tuple
    _ptpccenv->district_man()->give_tuple(prdist);
    return (RCOK);
}

w_rc_t upd_cust_pay_action_impl::trx_exec() 
{
    assert (_ptpccenv);
    TRACE( TRACE_TRX_FLOW, "upd customer\n");
    _pin.describe(0);

    // get table tuple from the cache
    row_impl<customer_t>* prcust = _ptpccenv->customer_man()->get_tuple();
    assert (prcust);
    rep_row_t areprow(_ptpccenv->customer_man()->ts());
    areprow.set(_ptpccenv->customer()->maxsize()); 
    prcust->_rep = &areprow;

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // TODO (ip) THOSE TWO SHOULD BE TWO DIFFERENT ACTIONS AND THE CLIENT SHOULD
    //           DECIDE, GIVEN THE _pin, WHICH OF THE TWO ACTIONS TO ENQUEUE
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // find the customer wh and d
    int c_w = (_pin._v_cust_wh_selection>85 ? _pin._home_wh_id : _pin._remote_wh_id);
    int c_d = (_pin._v_cust_wh_selection>85 ? _pin._home_d_id : _pin._remote_d_id);

    if (_pin._v_cust_ident_selection <= 60) {

        // if (ppin->_c_id == 0) {

        /* 3a. if no customer selected already use the index on the customer name */

        /* SELECT  c_id, c_first
         * FROM customer
         * WHERE c_last = :c_last AND c_w_id = :c_w_id AND c_d_id = :c_d_id
         * ORDER BY c_first
         *
         * plan: index only scan on "C_NAME_INDEX"
         */

        assert (_pin._v_cust_ident_selection <= 60);
        assert (_pin._c_id == 0); // (ip) just checks the generator output

        rep_row_t lowrep(_ptpccenv->customer_man()->ts());
        rep_row_t highrep(_ptpccenv->customer_man()->ts());

        index_scan_iter_impl<customer_t>* c_iter;
        TRACE( TRACE_TRX_FLOW, "App: %d PAY:cust-get-iter-by-name-index (%s)\n", 
               _tid, _pin._c_last);
        W_DO(_ptpccenv->customer_man()->cust_get_iter_by_index(_ptpccenv->db(), 
                                                               c_iter, prcust, 
                                                               lowrep, highrep,
                                                               c_w, c_d, 
                                                               _pin._c_last));

        vector<int> v_c_id;
        int a_c_id = 0;
        int count = 0;
        bool eof;

        W_DO(c_iter->next(_ptpccenv->db(), eof, *prcust));
        while (!eof) {
            count++;
            prcust->get_value(0, a_c_id);
            v_c_id.push_back(a_c_id);

            TRACE( TRACE_TRX_FLOW, "App: %d PAY:cust-iter-next (%d)\n", 
                   _tid, a_c_id);
            W_DO(c_iter->next(_ptpccenv->db(), eof, *prcust));
        }
        delete c_iter;
        assert (count);

        /* find the customer id in the middle of the list */
        _pin._c_id = v_c_id[(count+1)/2-1];
    }
    assert (_pin._c_id>0);

    /* 3. retrieve customer for update */

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
           _tid, c_w, c_d, _pin._c_id);

    W_DO(_ptpccenv->customer_man()->cust_index_probe_nl(_ptpccenv->db(), 
                                                        prcust, 
                                                        _pin._c_id, 
                                                        c_w, 
                                                        c_d));
    
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
    acust.C_BALANCE -= _pin._h_amount;
    acust.C_YTD_PAYMENT += _pin._h_amount;
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
                _pin._c_id, c_d, c_w, _pin._home_d_id, 
                _pin._home_wh_id, _pin._h_amount);

        int len = strlen(c_new_data_1);
        strncat(c_new_data_1, acust.C_DATA_1, 250-len);
        strncpy(c_new_data_2, &acust.C_DATA_1[250-len], len);
        strncpy(c_new_data_2, acust.C_DATA_2, 250-len);

        TRACE( TRACE_TRX_FLOW, "App: %d PAY:cust-update-tuple\n", _tid);
        W_DO(_ptpccenv->customer_man()->cust_update_tuple(_ptpccenv->db(), 
                                                          prcust, 
                                                          acust, 
                                                          c_new_data_1, 
                                                          c_new_data_2));
    }
    else { /* good customer */
        TRACE( TRACE_TRX_FLOW, "App: %d PAY:cust-update-tuple\n", _tid);
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

w_rc_t ins_hist_pay_action_impl::trx_exec() 
{
    assert (_ptpccenv);
    TRACE( TRACE_TRX_FLOW, "ins history\n");
    _pin.describe(0);

    // get table tuple from the cache
    row_impl<history_t>* prhist = _ptpccenv->history_man()->get_tuple();
    assert (prhist);
    rep_row_t areprow(_ptpccenv->history_man()->ts());
    areprow.set(_ptpccenv->history()->maxsize()); 
    prhist->_rep = &areprow;

    // find the customer wh and d
    int c_w = (_pin._v_cust_wh_selection>85 ? _pin._home_wh_id : _pin._remote_wh_id);
    int c_d = (_pin._v_cust_wh_selection>85 ? _pin._home_d_id : _pin._remote_d_id);

    /* INSERT INTO history
     * VALUES (:c_id, :c_d_id, :c_w_id, :d_id, :w_id, 
     *         :curr_tmstmp, :ih_amount, :h_data)
     */

    tpcc_history_tuple ahist;
    sprintf(ahist.H_DATA, "%s   %s", _awh.W_NAME, _adist.D_NAME);
    ahist.H_DATE = time(NULL);

    prhist->set_value(0, _pin._c_id);
    prhist->set_value(1, c_d);
    prhist->set_value(2, c_w);
    prhist->set_value(3, _pin._home_d_id);
    prhist->set_value(4, _pin._home_wh_id);
    prhist->set_value(5, ahist.H_DATE);
    prhist->set_value(6, _pin._h_amount * 100.0);
    prhist->set_value(7, ahist.H_DATA);

    TRACE( TRACE_TRX_FLOW, "App: %d PAY:hist-add-tuple\n", _tid);
    W_DO(_ptpccenv->history_man()->add_tuple(_ptpccenv->db(), prhist));

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prhist->print_tuple();
#endif

    // give back the tuple
    _ptpccenv->history_man()->give_tuple(prhist);
    return (RCOK);
}



EXIT_NAMESPACE(dora);


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
    TRACE( TRACE_TRX_FLOW, "%d. DORA - PAYMENT...\n", xct_id);         

    tid_t atid;   

    // 1. Initiate transaction
    W_DO(_pssm->begin_xct(atid));
    xct_t* pxct = ss_m::tid_to_xct(atid);    
    assert (pxct);

    // 2. Setup the next RVP
    midway_pay_rvp* rvp = new midway_pay_rvp(atid, pxct,
                                             &atrt, this, apin); // PH1 consists of 3 packets
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

    upd_cust_pay_action_impl* p_upd_cust_pay_action = _g_dora->get_upd_cust_pay_action();
    assert (p_upd_cust_pay_action);
    p_upd_cust_pay_action->set_input(atid, pxct, rvp, this, apin);
    rvp->set_puc(p_upd_cust_pay_action);
    p_upd_cust_pay_action->_m_rvp=rvp;
    if (_g_dora->cus()->enqueue(p_upd_cust_pay_action, apin._home_wh_id)) { // SF CUSTOMER partitions
            TRACE( TRACE_DEBUG, "Problem in enqueueing UPD_CUST_PAY\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
    }

    upd_dist_pay_action_impl* p_upd_dist_pay_action = _g_dora->get_upd_dist_pay_action();
    assert (p_upd_dist_pay_action);
    p_upd_dist_pay_action->set_input(atid, pxct, rvp, this, apin);
    rvp->set_pud(p_upd_dist_pay_action);
    p_upd_dist_pay_action->_m_rvp=rvp;
    if (_g_dora->dis()->enqueue(p_upd_dist_pay_action, 0)) { // One DISTRICT partition
            TRACE( TRACE_DEBUG, "Problem in enqueueing UPD_DIST_PAY\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
    }

    upd_wh_pay_action_impl*   p_upd_wh_pay_action   = _g_dora->get_upd_wh_pay_action();
    assert (p_upd_wh_pay_action);
    p_upd_wh_pay_action->set_input(atid, pxct, rvp, this, apin);
    rvp->set_puw(p_upd_wh_pay_action);
    p_upd_wh_pay_action->_m_rvp=rvp;
    if (_g_dora->whs()->enqueue(p_upd_wh_pay_action, 0)) { // One WAREHOUSE partition
            TRACE( TRACE_DEBUG, "Problem in enqueueing UPD_WH_PAY\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
    }

    // 4. detatch self from xct
    me()->detach_xct(pxct);
    
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
                                           &atrt, this);
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
 * @note: The functions below are private, the corresponding run_XXX are
 *        their public wrappers. The run_XXX are required because they
 *        do the trx abort in case something goes wrong inside the body
 *        of each of the transactions.
 *
 ********************************************************************/


// uncomment the line below if want to dump (part of) the trx results
//#define PRINT_TRX_RESULTS


/******************************************************************** 
 *
 * DORA - TPC-C NEW_ORDER
 *
 ********************************************************************/

w_rc_t ShoreTPCCEnv::_dora_new_order(new_order_input_t* pnoin, 
                                     const int xct_id, 
                                     trx_result_tuple_t& trt)
{
    assert (0); // TODO - TEST
    return (RCOK);

} // EOF: NEW_ORDER




/******************************************************************** 
 *
 * TPC-C ORDER STATUS
 *
 * Input: w_id, d_id, c_id (use c_last if set to null), c_last
 *
 * @note: Read-Only trx
 *
 ********************************************************************/


w_rc_t ShoreTPCCEnv::_dora_order_status(order_status_input_t* pstin, 
                                        const int xct_id, 
                                        trx_result_tuple_t& trt)
{
    assert (0); // TODO - TEST
    return (RCOK);

}; // EOF: ORDER-STATUS



/******************************************************************** 
 *
 * TPC-C DELIVERY
 *
 * Input data: w_id, carrier_id
 *
 * @note: Delivers one new_order (undelivered order) from each district
 *
 ********************************************************************/


w_rc_t ShoreTPCCEnv::_dora_delivery(delivery_input_t* pdin, 
                                    const int xct_id, 
                                    trx_result_tuple_t& trt)
{
    assert (0); // TODO - TEST
    return (RCOK);
}




/******************************************************************** 
 *
 * TPC-C STOCK LEVEL
 *
 * Input data: w_id, d_id, threshold
 *
 * @note: Read-only transaction
 *
 ********************************************************************/

w_rc_t ShoreTPCCEnv::_dora_stock_level(stock_level_input_t* pslin, 
                                       const int xct_id, 
                                       trx_result_tuple_t& trt)
{
    assert (0); // TODO - TEST
    return (RCOK);
}



EXIT_NAMESPACE(tpcc);
