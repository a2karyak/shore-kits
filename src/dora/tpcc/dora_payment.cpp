/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_payment.cpp
 *
 *  @brief:  DORA TPC-C PAYMENT
 *
 *  @note:   Implementation of RVPs and Actions that synthesize 
 *           the TPC-C Payment trx according to DORA
 *
 *  @author: Ippokratis Pandis, Oct 2008
 */

#include "dora/tpcc/dora_payment.h"
#include "dora/tpcc/dora_tpcc.h"


using namespace dora;
using namespace shore;
using namespace tpcc;



ENTER_NAMESPACE(dora);


//
// RVPS
//
// (1) midway_pay_rvp
// (2) final_pay_rvp
//


/******************************************************************** 
 *
 * PAYMENT MIDWAY RVP
 *
 ********************************************************************/

w_rc_t midway_pay_rvp::run() 
{
    // 1. Setup the next RVP
    assert (_xct);
    final_pay_rvp* frvp = new (_ptpccenv->_final_pay_rvp_pool) final_pay_rvp;
    assert (frvp);
    frvp->set(_tid,_xct,_xct_id,_result,_ptpccenv,&_ptpccenv->_final_pay_rvp_pool);
    frvp->copy_actions(_actions);

    // 2. Generate and enqueue action
    ins_hist_pay_action* ins_hist_pay = new (_ptpccenv->_ins_hist_pay_pool) ins_hist_pay_action;
    assert (ins_hist_pay);
    ins_hist_pay->set(_tid,_xct,frvp,_pin,_awh,_adist,_ptpccenv,&_ptpccenv->_ins_hist_pay_pool);
    frvp->add_action(ins_hist_pay);

    int mypartition = _pin._home_wh_id-1;

    // Q: (ip) does it have to get this lock?

    // HIS_PART_CS
    TRACE( TRACE_TRX_FLOW, "Next phase (%d)\n", _tid);
    CRITICAL_SECTION(his_part_cs, _ptpccenv->his(mypartition)->_enqueue_lock);

    if (_ptpccenv->his()->enqueue(ins_hist_pay, mypartition)) { // (SF) HISTORY partitions
            TRACE( TRACE_DEBUG, "Problem in enqueueing INS_HIST_PAY\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
    }

    return (RCOK);
}




/******************************************************************** 
 *
 * PAYMENT FINAL RVP
 *
 ********************************************************************/

w_rc_t final_pay_rvp::run() {
    assert (_ptpccenv); 
    return (_run(_ptpccenv)); 
}

void final_pay_rvp::upd_committed_stats() 
{
    assert (_ptpccenv);
    if (_ptpccenv->get_measure() != MST_MEASURE) {
        return;
    }

    _ptpccenv->get_total_tpcc_stats()->inc_pay_com();
    _ptpccenv->get_session_tpcc_stats()->inc_pay_com();
    _ptpccenv->get_env_stats()->inc_trx_com();    
}                     

void final_pay_rvp::upd_aborted_stats() 
{
    assert (_ptpccenv);
    if (_ptpccenv->get_measure() != MST_MEASURE) {
        return;
    }

    _ptpccenv->get_total_tpcc_stats()->inc_pay_att();
    _ptpccenv->get_session_tpcc_stats()->inc_pay_att();
    _ptpccenv->get_env_stats()->inc_trx_att();
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

const bool pay_action::trx_acq_locks()
{
    // all the Payment trxs are (EX) probes to a single tuple
    assert (_partition);
    LockRequestVec alrvec;
    LockRequest alr(&_down,DL_CC_EXCL);
    alrvec.push_back(alr);    
    return (_partition->acquire(_tid,alrvec));
}

w_rc_t upd_wh_pay_action::trx_exec() 
{
    assert (_ptpccenv);

    // get table tuple from the cache
    row_impl<warehouse_t>* prwh = _ptpccenv->warehouse_man()->get_tuple();
    assert (prwh);
    rep_row_t areprow(_ptpccenv->warehouse_man()->ts());
    areprow.set(_ptpccenv->warehouse()->maxsize()); 
    prwh->_rep = &areprow;

    w_rc_t e = RCOK;

    /* 1. retrieve warehouse for update */
    TRACE( TRACE_TRX_FLOW, 
           "App: %d PAY:warehouse-idx-probe-nl (%d)\n", 
           _tid, _pin._home_wh_id);

    e = _ptpccenv->warehouse_man()->wh_index_probe_nl(_ptpccenv->db(), prwh, 
                                                      _pin._home_wh_id);      
    if (e.is_error()) { goto done; }

    /* UPDATE warehouse SET w_ytd = wytd + :h_amount
     * WHERE w_id = :w_id
     *
     * SELECT w_name, w_street_1, w_street_2, w_city, w_state, w_zip
     * FROM warehouse
     * WHERE w_id = :w_id
     *
     * plan: index probe on "W_INDEX"
     */

    TRACE( TRACE_TRX_FLOW, "App: %d PAY:wh-update-ytd-nl (%d)\n", 
           _tid, _pin._home_wh_id);
    e = _ptpccenv->warehouse_man()->wh_update_ytd_nl(_ptpccenv->db(), 
                                                     prwh, 
                                                     _pin._h_amount);
    if (e.is_error()) { goto done; }

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


done:
    // give back the tuple
    _ptpccenv->warehouse_man()->give_tuple(prwh);
    return (e);
}

w_rc_t upd_dist_pay_action::trx_exec() 
{
    assert (_ptpccenv);

    // get table tuple from the cache
    row_impl<district_t>* prdist = _ptpccenv->district_man()->get_tuple();
    assert (prdist);
    rep_row_t areprow(_ptpccenv->district_man()->ts());
    areprow.set(_ptpccenv->district()->maxsize()); 
    prdist->_rep = &areprow;

    w_rc_t e = RCOK;

    /* 1. retrieve district for update */
    TRACE( TRACE_TRX_FLOW, 
           "App: %d PAY:district-idx-probe-nl (%d) (%d)\n", 
           _tid, _pin._home_wh_id, _pin._home_d_id);

    e = _ptpccenv->district_man()->dist_index_probe_nl(_ptpccenv->db(), prdist,
                                                       _pin._home_wh_id, _pin._home_d_id);    
    if (e.is_error()) { goto done; }


    /* UPDATE district SET d_ytd = d_ytd + :h_amount
     * WHERE d_id = :d_id AND d_w_id = :w_id
     *
     * SELECT d_street_1, d_street_2, d_city, d_state, d_zip, d_name
     * FROM district
     * WHERE d_id = :d_id AND d_w_id = :w_id
     *
     * plan: index probe on "D_INDEX"
     */

    TRACE( TRACE_TRX_FLOW, "App: %d PAY:distr-upd-ytd-nl (%d) (%d)\n", 
           _tid, _pin._home_wh_id, _pin._home_d_id);
    e = _ptpccenv->district_man()->dist_update_ytd_nl(_ptpccenv->db(), 
                                                      prdist, 
                                                      _pin._h_amount);
    if (e.is_error()) { goto done; }

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

done:
    // give back the tuple
    _ptpccenv->district_man()->give_tuple(prdist);
    return (e);
}


w_rc_t upd_cust_pay_action::trx_exec() 
{
    assert (_ptpccenv);

    // get table tuple from the cache
    row_impl<customer_t>* prcust = _ptpccenv->customer_man()->get_tuple();
    assert (prcust);
    rep_row_t areprow(_ptpccenv->customer_man()->ts());
    areprow.set(_ptpccenv->customer()->maxsize()); 
    prcust->_rep = &areprow;

    w_rc_t e = RCOK;

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

        e = _ptpccenv->customer_man()->cust_get_iter_by_index(_ptpccenv->db(), 
                                                              c_iter, prcust, 
                                                              lowrep, highrep,
                                                              c_w, c_d, 
                                                              _pin._c_last);
        if (e.is_error()) { goto done; }
    
        vector<int> v_c_id;
        int a_c_id = 0;
        int count = 0;
        bool eof;

        e = c_iter->next(_ptpccenv->db(), eof, *prcust);
        if (e.is_error()) { goto done; }
        while (!eof) {
            count++;
            prcust->get_value(0, a_c_id);
            v_c_id.push_back(a_c_id);

            TRACE( TRACE_TRX_FLOW, "App: %d PAY:cust-iter-next (%d)\n", 
                   _tid, a_c_id);
            e = c_iter->next(_ptpccenv->db(), eof, *prcust);
            if (e.is_error()) { goto done; }
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
           "App: %d PAY:cust-idx-probe-forupdate-nl (%d) (%d) (%d)\n", 
           _tid, c_w, c_d, _pin._c_id);

    e = _ptpccenv->customer_man()->cust_index_probe_nl(_ptpccenv->db(), prcust, 
                                                       c_w, c_d, _pin._c_id);
    if (e.is_error()) { goto done; }    
    
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

        TRACE( TRACE_TRX_FLOW, "App: %d PAY:cust-update-tuple-nl\n", _tid);
        e = _ptpccenv->customer_man()->cust_update_tuple_nl(_ptpccenv->db(), 
                                                            prcust, 
                                                            acust, 
                                                            c_new_data_1, 
                                                            c_new_data_2);
        if (e.is_error()) { goto done; }
    }
    else { /* good customer */
        TRACE( TRACE_TRX_FLOW, "App: %d PAY:cust-update-tuple-nl\n", _tid);
        e = _ptpccenv->customer_man()->cust_update_tuple_nl(_ptpccenv->db(), 
                                                            prcust, 
                                                            acust, 
                                                            NULL, 
                                                            NULL);
        if (e.is_error()) { goto done; }
    }

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

w_rc_t ins_hist_pay_action::trx_exec() 
{
    assert (_ptpccenv);

    // get table tuple from the cache
    row_impl<history_t>* prhist = _ptpccenv->history_man()->get_tuple();
    assert (prhist);
    rep_row_t areprow(_ptpccenv->history_man()->ts());
    areprow.set(_ptpccenv->history()->maxsize()); 
    prhist->_rep = &areprow;

    w_rc_t e = RCOK;

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
    e = _ptpccenv->history_man()->add_tuple(_ptpccenv->db(), prhist);
    if (e.is_error()) { goto done; }

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prhist->print_tuple();
#endif

done:
    // give back the tuple
    _ptpccenv->history_man()->give_tuple(prhist);
    return (e);
}


EXIT_NAMESPACE(dora);
