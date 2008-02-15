/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_tpcc_env.cpp
 *
 *  @brief Declaration of the Shore TPC-C environment (database)
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "stages/tpcc/shore/shore_tpcc_env.h"


using namespace shore;

ENTER_NAMESPACE(tpcc);


/** Exported variables */

ShoreTPCCEnv* shore_env;


/** Exported functions */


/********
 ******** Caution: The functions below should be invoked inside
 ********          the context of a smthread
 ********/


/****************************************************************** 
 *
 * @fn:    loaddata()
 *
 * @brief: Loads the data for all the TPCC tables, given the current
 *         scaling factor value. During the loading the SF cannot be
 *         changed.
 *
 ******************************************************************/

w_rc_t ShoreTPCCEnv::loaddata() 
{
    /* 0. lock the loading status and the scaling factor */
    critical_section_t load_cs(_load_mutex);
    if (_loaded) {
        TRACE( TRACE_ALWAYS, 
               "Env already loaded. Doing nothing...\n");
        return (RCOK);
    }        
    critical_section_t scale_cs(_scaling_mutex);


    /* 1. create the loader threads */
    tpcc_table_t* ptable = NULL;
    int num_tbl = _table_list.size();
    int cnt = 0;
    guard<tpcc_loading_smt_t> loaders[SHORE_TPCC_TABLES];
    time_t tstart = time(NULL);

    for(tpcc_table_list_iter table_iter = _table_list.begin(); 
        table_iter != _table_list.end(); table_iter++)
        {
            ptable = *table_iter;

            char fname[MAX_FILENAME_LEN];
            strcpy(fname, SHORE_TPCC_DATA_DIR);
            strcat(fname, "/");
            strcat(fname, ptable->name());
            strcat(fname, ".dat");
            TRACE( TRACE_DEBUG, "%d. Loading (%s) from (%s)\n", 
                   ++cnt, ptable->name(), fname);
            time_t ttablestart = time(NULL);
            w_rc_t e = ptable->load_from_file(_pssm, fname);
            time_t ttablestop = time(NULL);
            if (e != RCOK) {
                TRACE( TRACE_ALWAYS, "%d. Error while loading (%s) *****\n",
                       cnt, ptable->name());
                return RC(se_ERROR_IN_LOAD);
            }

            TRACE( TRACE_DEBUG, "Table (%s) loaded in (%d) secs...\n",
                   ptable->name(), (ttablestop - ttablestart));

            //            loaders[cnt] = new tpcc_loading_smt_t(_pssm, ptable, 
            //                                                  _scaling_factor, cnt++);
        }

#if 0
#if 1
    /* 3. fork the loading threads */
    for(int i=0; i<num_tbl; i++) {
	loaders[i]->fork();
    }

    /* 4. join the loading threads */
    for(int i=0; i<num_tbl; i++) {
	loaders[i]->join();        
        if (loaders[i]->rv()) {
            cerr << "~~~~ Error at loader " << i << endl;
            assert (false);
            return RC(se_ERROR_IN_LOAD);
        }
    }    
#else
    /* 3. fork & join the loading threads SERIALLY */
    for(int i=0; i<num_tbl; i++) {
	loaders[i]->fork();
	loaders[i]->join();        
        if (loaders[i]->rv()) {
            cerr << "~~~~ Error at loader " << i << endl;
            assert (false);
            return RC(se_ERROR_IN_LOAD);
        }        
    }
#endif
#endif
    time_t tstop = time(NULL);

    /* 5. print stats */
    TRACE( TRACE_STATISTICS, "Loading finished in (%d) secs...\n",
           (tstop - tstart));
    TRACE( TRACE_STATISTICS, "%d tables loaded...\n", num_tbl);

    /* 6. notify that the env is loaded */
    _loaded = true;

    return (RCOK);
}



/****************************************************************** 
 *
 * @fn:    check_consistency()
 *
 * @brief: Iterates over all tables and checks consistency between
 *         the values stored in the base table (file) and the 
 *         corresponding indexes.
 *
 ******************************************************************/

w_rc_t ShoreTPCCEnv::check_consistency()
{
    /* 1. create the checker threads */
    tpcc_table_t* ptable = NULL;
    int num_tbl = _table_list.size();
    int cnt = 0;
    guard<tpcc_checking_smt_t> checkers[SHORE_TPCC_TABLES];

    for(tpcc_table_list_iter table_iter = _table_list.begin(); 
        table_iter != _table_list.end(); table_iter++)
        {
            ptable = *table_iter;
            checkers[cnt] = new tpcc_checking_smt_t(_pssm, ptable, cnt++);
        }

#if 0
    /* 2. fork the threads */
    cnt = 0;
    time_t tstart = time(NULL);
    for(int i=0; i<num_tbl; i++) {
	checkers[i]->fork();
    }

    /* 3. join the threads */
    cnt = 0;
    for(int i=0; i < num_tbl; i++) {
	checkers[i]->join();
    }    
    time_t tstop = time(NULL);
#else
    /* 2. fork & join the threads SERIALLY */
    cnt = 0;
    time_t tstart = time(NULL);
    for(int i=0; i<num_tbl; i++) {
	checkers[i]->fork();
	checkers[i]->join();
    }
    time_t tstop = time(NULL);
#endif
    /* 4. print stats */
    TRACE( TRACE_DEBUG, "Checking finished in (%d) secs...\n",
           (tstop - tstart));
    TRACE( TRACE_DEBUG, "%d tables checked...\n", num_tbl);
    return (RCOK);
}



void ShoreTPCCEnv::set_qf(const int aQF)
{
    if ((aQF >= 0) && (aQF <= _scaling_factor)) {
        critical_section_t cs(_queried_mutex);
        TRACE( TRACE_ALWAYS, "New Queried factor: %d\n", aQF);
        _queried_factor = aQF;
    }
    else {
        TRACE( TRACE_ALWAYS, "Invalid queried factor input: %d\n", aQF);
    }
}



/******************************************************************** 
 *
 * TPC-C TRXS
 *
 ********************************************************************/

w_rc_t ShoreTPCCEnv::xct_order_status(order_status_input_t* pstin, 
                                      const int xct_id, 
                                      trx_result_tuple_t& trt)
{ return (RCOK); }

w_rc_t ShoreTPCCEnv::xct_delivery(delivery_input_t* pdin, 
                                  const int xct_id, 
                                  trx_result_tuple_t& trt)
{ return (RCOK); }

w_rc_t ShoreTPCCEnv::xct_stock_level(stock_level_input_t* plin, 
                                     const int xct_id, 
                                     trx_result_tuple_t& trt)
{ return (RCOK); }



/******************************************************************** 
 *
 * TPC-C PAYMENT
 *
 ********************************************************************/


w_rc_t ShoreTPCCEnv::xct_payment(payment_input_t* ppin, 
                                 const int xct_id, 
                                 trx_result_tuple_t& trt)
{
    // ensure a valid environment    
    assert (ppin);
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);


    // payment trx touches 4 tables: 
    // warehouse, district, customer, and history
    table_row_t rwh;
    table_row_t rdist;
    table_row_t rcust;
    table_row_t rhist;
    trt.reset(UNSUBMITTED, xct_id);

    /* 0. initiate transaction */
    W_DO(_pssm->begin_xct());

    /* 1. retrieve warehouse for update */
    TRACE( TRACE_TRX_FLOW, 
           "App: %d PAY:warehouse-index-probe\n", xct_id, ppin->_home_wh_id);
    W_DO(_warehouse.index_probe_forupdate(_pssm, &rwh, ppin->_home_wh_id));   

    /* 2. retrieve district for update */
    TRACE( TRACE_TRX_FLOW, 
           "App: %d PAY:warehouse-index-probe\n", 
           xct_id, ppin->_home_wh_id);
    W_DO(_district.index_probe_forupdate(_pssm, 
                                         &rdist,
                                         ppin->_home_d_id, 
                                         ppin->_home_wh_id));

    // find the customer wh and d
    int c_w = ( ppin->_v_cust_ident_selection > 85 ? ppin->_home_wh_id : ppin->_remote_wh_id);
    int c_d = ( ppin->_v_cust_ident_selection > 85 ? ppin->_home_d_id : ppin->_remote_d_id);

    /* 3. retrieve customer for update */
    if (ppin->_c_id == 0) {

        /* 3a. if no customer selected already use the index on the customer name */

	/* SELECT  c_id, c_first
	 * FROM customer
	 * WHERE c_last = :c_last AND c_w_id = :c_w_id AND c_d_id = :c_d_id
	 * ORDER BY c_first
	 *
	 * plan: index only scan on "C_NAME_INDEX"
	 */

        index_scan_iter_impl* c_iter;
        TRACE( TRACE_TRX_FLOW, "App: %d, PAY:get-iter-by-index\n", xct_id);
	W_DO(_customer.get_iter_by_index(_pssm, c_iter, &rcust, 
                                         c_w, c_d, ppin->_c_last));

	int   c_id_list[17];
	int   count = 0;
	bool  eof;

        TRACE( TRACE_TRX_FLOW, "App: %d, PAY:iter-next\n", xct_id);
	W_DO(c_iter->next(_pssm, eof, rcust));
	while (!eof) {
	    rcust.get_value(0, c_id_list[count++]);            
            TRACE( TRACE_TRX_FLOW, "App: %d, PAY:iter-next\n", xct_id);
	    W_DO(c_iter->next(_pssm, eof, rcust));
	}
	delete c_iter;

	/* find the customer id in the middle of the list */
	ppin->_c_id = c_id_list[(count+1)/2-1];
    }

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
           "App: %d, PAY:index-probe-forupdate (%d) (%d) (%d)\n", 
           xct_id, ppin->_c_id, c_w, c_d);
    W_DO(_customer.index_probe_forupdate(_pssm, &rcust, ppin->_c_id, 
                                         c_w, c_d));

    double c_balance, c_ytd_payment;
    int    c_payment_cnt;
    tpcc_customer_tuple acust;

    // retrieve customer
    rcust.get_value(3,  acust.C_FIRST, 17);
    rcust.get_value(4,  acust.C_MIDDLE, 3);
    rcust.get_value(5,  acust.C_LAST, 17);
    rcust.get_value(6,  acust.C_STREET_1, 21);
    rcust.get_value(7,  acust.C_STREET_2, 21);
    rcust.get_value(8,  acust.C_CITY, 21);
    rcust.get_value(9,  acust.C_STATE, 3);
    rcust.get_value(10, acust.C_ZIP, 10);
    rcust.get_value(11, acust.C_PHONE, 17);
    rcust.get_value(12, acust.C_SINCE);
    rcust.get_value(13, acust.C_CREDIT, 3);
    rcust.get_value(14, acust.C_CREDIT_LIM);
    rcust.get_value(15, acust.C_DISCOUNT);
    rcust.get_value(16, acust.C_BALANCE);
    rcust.get_value(17, acust.C_YTD_PAYMENT);
    rcust.get_value(18, acust.C_LAST_PAYMENT);
    rcust.get_value(19, acust.C_PAYMENT_CNT);
    rcust.get_value(20, acust.C_DATA_1, 251);
    rcust.get_value(21, acust.C_DATA_2, 251);

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

        TRACE( TRACE_TRX_FLOW, 
               "App: %d, PAY:index-probe-forupdate (%d) (%d) (%d)\n", 
               xct_id, ppin->_c_id, c_w, c_d);        
	W_DO(_customer.index_probe_forupdate(_pssm, &rcust, ppin->_c_id, c_w, c_d));

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

        TRACE( TRACE_TRX_FLOW, "App: %d, PAY:update-tuple\n", xct_id);
	W_DO(_customer.update_tuple(_pssm, &rcust, acust, c_new_data_1, c_new_data_2));
    }
    else { /* good customer */
        TRACE( TRACE_TRX_FLOW, "App: %d, PAY:update-tuple\n", xct_id);
	W_DO(_customer.update_tuple(_pssm, &rcust, acust, NULL, NULL));
    }

    /* UPDATE district SET d_ytd = d_ytd + :h_amount
     * WHERE d_id = :d_id AND d_w_id = :w_id
     *
     * plan: index probe on "D_INDEX"
     */

    TRACE( TRACE_TRX_FLOW, "App: %d, PAY:update-ytd1 (%d)\n", xct_id, ppin->_home_wh_id);
    W_DO(_district.update_ytd(_pssm, &rdist, ppin->_home_d_id, ppin->_home_wh_id, ppin->_h_amount));

    /* SELECT d_street_1, d_street_2, d_city, d_state, d_zip, d_name
     * FROM district
     * WHERE d_id = :d_id AND d_w_id = :w_id
     *
     * plan: index probe on "D_INDEX"
     */

    TRACE( TRACE_TRX_FLOW, "App: %d, PAY:index-probe (%d) (%d)\n", 
           xct_id, ppin->_home_d_id, ppin->_home_wh_id);
    W_DO(_district.index_probe(_pssm, &rdist, ppin->_home_d_id, ppin->_home_wh_id));

    tpcc_district_tuple adistr;
    rdist.get_value(2, adistr.D_NAME, 11);
    rdist.get_value(3, adistr.D_STREET_1, 21);
    rdist.get_value(4, adistr.D_STREET_2, 21);
    rdist.get_value(5, adistr.D_CITY, 21);
    rdist.get_value(6, adistr.D_STATE, 3);
    rdist.get_value(7, adistr.D_ZIP, 10);

    /* UPDATE warehouse SET w_ytd = wytd + :h_amount
     * WHERE w_id = :w_id
     *
     * plan: index probe on "W_INDEX"
     */

    TRACE( TRACE_TRX_FLOW, "App: %d, PAY:update-ytd2 (%d)\n", xct_id, ppin->_home_wh_id);
    W_DO(_warehouse.update_ytd(_pssm, &rwh, ppin->_home_wh_id, ppin->_h_amount));

    tpcc_warehouse_tuple awh;
    rwh.get_value(1, awh.W_NAME, 11);
    rwh.get_value(2, awh.W_STREET_1, 21);
    rwh.get_value(3, awh.W_STREET_2, 21);
    rwh.get_value(4, awh.W_CITY, 21);
    rwh.get_value(5, awh.W_STATE, 3);
    rwh.get_value(6, awh.W_ZIP, 10);


    /* INSERT INTO history
     * VALUES (:c_id, :c_d_id, :c_w_id, :d_id, :w_id, :curr_tmstmp, :ih_amount, :h_data)
     */

    tpcc_history_tuple ahist;
    sprintf(ahist.H_DATA, "%s   %s", awh.W_NAME, adistr.D_NAME);
    ahist.H_DATE = time(NULL);
    rhist.set_value(0, ppin->_c_id);
    rhist.set_value(1, c_d);
    rhist.set_value(2, c_w);
    rhist.set_value(3, ppin->_home_d_id);
    rhist.set_value(4, ppin->_home_wh_id);
    rhist.set_value(5, ahist.H_DATE);
    rhist.set_value(6, ppin->_h_amount * 100.0);
    rhist.set_value(7, ahist.H_DATA);

    TRACE( TRACE_TRX_FLOW, "App: %d, PAY:add-tuple\n", xct_id);
    W_DO(_history.add_tuple(_pssm, &rhist));

    /* 4. commit */
    W_DO(_pssm->commit_xct());

    // if we reached this point everything went ok
    trt.set_state(COMMITTED);

    return (RCOK);

} // EOF: PAYMENT




/******************************************************************** 
 *
 * TPC-C NEW_ORDER
 *
 ********************************************************************/

w_rc_t ShoreTPCCEnv::xct_new_order(new_order_input_t* pnoin, 
                                   const int xct_id, 
                                   trx_result_tuple_t& trt)
{
    // ensure a valid environment
    assert (pnoin);
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    // get a timestamp
    time_t tstamp = time(NULL);

    // new_order trx touches 8 tables:
    // warehouse, district, customer, neworder, order, item, stock, orderline
    table_row_t rwh;
    table_row_t rdist;
    table_row_t rcust;
    table_row_t rno;
    table_row_t rord;
    table_row_t ritem;
    table_row_t rst;
    table_row_t rol;
    trt.reset(UNSUBMITTED, xct_id);

    /* 0. initiate transaction */
    W_DO(_pssm->begin_xct());


    /* SELECT c_discount, c_last, c_credit, w_tax
     * FROM customer, warehouse
     * WHERE w_id = :w_id AND c_w_id = w_id AND c_d_id = :d_id AND c_id = :c_id
     *
     * plan: index probe on "W_INDEX", index probe on "C_INDEX"
     */

    /* 1. retrieve warehouse for update */
    TRACE( TRACE_TRX_FLOW, 
           "App: %d NO:warehouse-index-probe (%d)\n", 
           xct_id, pnoin->_wh_id);
    W_DO(_warehouse.index_probe(_pssm, &rwh, pnoin->_wh_id));

    /* 2. retrieve district for update */
    TRACE( TRACE_TRX_FLOW, 
           "App: %d NO:district-index-probe (%d) (%d)\n", 
           xct_id, pnoin->_wh_id, pnoin->_d_id);
    W_DO(_district.index_probe_forupdate(_pssm, &rdist, 
                                         pnoin->_wh_id, pnoin->_d_id));

    /* 3. retrieve customer */
    TRACE( TRACE_TRX_FLOW, 
           "App: %d NO:customer-index-probe (%d) (%d) (%d)\n", 
           xct_id, pnoin->_wh_id, pnoin->_d_id, pnoin->_c_id);
    W_DO(_customer.index_probe(_pssm, &rcust, pnoin->_c_id, 
                               pnoin->_wh_id, pnoin->_d_id));

    tpcc_warehouse_tuple awh;
    tpcc_customer_tuple  acust;

    rwh.get_value(7, awh.W_TAX);
    rcust.get_value(15, acust.C_DISCOUNT);
    rcust.get_value(13, acust.C_CREDIT, 3);
    rcust.get_value(5, acust.C_LAST, 17);


    /* SELECT d_tax, d_next_o_id
     * FROM district
     * WHERE d_id = :d_id AND d_w_id = :w_id
     *
     * plan: index probe on "D_INDEX"
     */

    TRACE( TRACE_TRX_FLOW, "App: %d NO:district-index-probe (%d) (%d)\n",
           xct_id, pnoin->_wh_id, pnoin->_d_id);

    tpcc_district_tuple adist;
    rdist.get_value(8, adist.D_TAX);
    rdist.get_value(10, adist.D_NEXT_O_ID);
    adist.D_NEXT_O_ID++;


    /* UPDATE district
     * SET d_next_o_id = :next_o_id+1
     * WHERE CURRENT OF dist_cur
     */

    TRACE( TRACE_TRX_FLOW, "App: %d NO:district-update-next-o-id\n", xct_id);
    W_DO(_district.update_next_o_id(_pssm, &rdist, adist.D_NEXT_O_ID));
    double total_amount = 0;
    int all_local = 0;

    for (int item_cnt = 0; item_cnt < pnoin->_ol_cnt; item_cnt++) {

        /* 4. for all items update item, stock, and order line */
	register int  ol_i_id = pnoin->items[item_cnt]._ol_i_id;
	register short ol_supply_w_id = pnoin->items[item_cnt]._ol_supply_wh_id;


	/* SELECT i_price, i_name, i_data
	 * FROM item
	 * WHERE i_id = :ol_i_id
	 *
	 * plan: index probe on "I_INDEX"
	 */

        tpcc_item_tuple aitem;
        TRACE( TRACE_TRX_FLOW, "App: %d NO:item-index-probe (%d)\n", 
               xct_id, ol_i_id);
	W_DO(_item.index_probe(_pssm, &ritem, ol_i_id));

        ritem.get_value(4, aitem.I_DATA, 51);
	ritem.get_value(3, aitem.I_PRICE);
	ritem.get_value(2, aitem.I_NAME, 25);

        double damount = aitem.I_PRICE * pnoin->items[item_cnt]._ol_quantity; 
        total_amount += damount;
        //	info->items[item_cnt].ol_amount = amount;


	/* SELECT s_quantity, s_remote_cnt, s_data, s_dist0, s_dist1, s_dist2, ...
	 * FROM stock
	 * WHERE s_i_id = :ol_i_id AND s_w_id = :ol_supply_w_id
	 *
	 * plan: index probe on "S_INDEX"
	 */

        tpcc_stock_tuple astock;
        TRACE( TRACE_TRX_FLOW, "App: %d NO:stock-index-probe (%d) (%d)\n", 
               xct_id, ol_i_id, ol_supply_w_id);
	W_DO(_stock.index_probe_forupdate(_pssm, &rst, ol_i_id, ol_supply_w_id));       

        rst.get_value(0, astock.S_I_ID);
        rst.get_value(1, astock.S_W_ID);
        rst.get_value(5, astock.S_YTD);
        astock.S_YTD += pnoin->items[item_cnt]._ol_quantity;
	rst.get_value(2, astock.S_REMOTE_CNT);        
        rst.get_value(3, astock.S_QUANTITY);
        astock.S_QUANTITY -= pnoin->items[item_cnt]._ol_quantity;
        if (astock.S_QUANTITY < 10) astock.S_QUANTITY += 91;
        rst.get_value(6+pnoin->_d_id, astock.S_DIST[6+pnoin->_d_id], 25);
	rst.get_value(16, astock.S_DATA, 51);

        char c_s_brand_generic;
	if (strstr(aitem.I_DATA, "ORIGINAL") != NULL && 
            strstr(astock.S_DATA, "ORIGINAL") != NULL)
	    c_s_brand_generic = 'B';
	else c_s_brand_generic = 'G';

	rst.get_value(4, astock.S_ORDER_CNT);
        astock.S_ORDER_CNT++;

	if (pnoin->_wh_id != ol_supply_w_id) {
            astock.S_REMOTE_CNT++;
	    all_local = 1;
	}


	/* UPDATE stock
	 * SET s_quantity = :s_quantity, s_order_cnt = :s_order_cnt
	 * WHERE s_w_id = :w_id AND s_i_id = :ol_i_id;
	 */

        TRACE( TRACE_TRX_FLOW, "App: %d NO:stock-update-tuple (%d) (%d) (%d)\n", 
               xct_id, astock.S_ORDER_CNT, astock.S_YTD, astock.S_REMOTE_CNT);
	W_DO(_stock.update_tuple(_pssm, &rst, &astock));


	/* INSERT INTO order_line
	 * VALUES (o_id, d_id, w_id, ol_ln, ol_i_id, supply_w_id,
	 *        '0001-01-01-00.00.01.000000', ol_quantity, iol_amount, dist)
	 */

	rol.set_value(0, adist.D_NEXT_O_ID);
	rol.set_value(1, pnoin->_d_id);
	rol.set_value(2, pnoin->_wh_id);
	rol.set_value(3, item_cnt+1);
	rol.set_value(4, ol_i_id);
	rol.set_value(5, ol_supply_w_id);
	rol.set_value(6, tstamp);
	rol.set_value(7, pnoin->items[item_cnt]._ol_quantity);
	rol.set_value(8, damount);
	rol.set_value(9, astock.S_DIST[6+pnoin->_d_id]);

        TRACE( TRACE_TRX_FLOW, "App: %d NO:add-tuple (%d)\n", 
               xct_id, adist.D_NEXT_O_ID);
	W_DO(_order_line.add_tuple(_pssm, &rol));

    } /* end for loop */


    /* 5. insert row to orders and new_order */

    /* INSERT INTO orders
     * VALUES (o_id, o_d_id, o_w_id, o_c_id, o_entry_d, o_ol_cnt, o_all_local)
     */
   
    rord.set_value(0, adist.D_NEXT_O_ID);
    rord.set_value(1, pnoin->_c_id);
    rord.set_value(2, pnoin->_d_id);
    rord.set_value(3, pnoin->_wh_id);
    rord.set_value(4, tstamp);
    rord.set_value(5, 0);
    rord.set_value(6, pnoin->_ol_cnt);
    rord.set_value(7, all_local);

    TRACE( TRACE_TRX_FLOW, "App: %d NO:add-tuple (%d)\n", 
           xct_id, adist.D_NEXT_O_ID);
    W_DO(_order.add_tuple(_pssm, &rord));


    /* INSERT INTO new_order VALUES (o_id, d_id, w_id)
     */

    rno.set_value(0, adist.D_NEXT_O_ID);
    rno.set_value(1, pnoin->_d_id);
    rno.set_value(2, pnoin->_wh_id);

    TRACE( TRACE_TRX_FLOW, "App: %d NO:add-tuple (%d) (%d) (%d)\n", 
           xct_id, adist.D_NEXT_O_ID, pnoin->_d_id, pnoin->_wh_id);
    W_DO(_new_order.add_tuple(_pssm, &rno));

    /* 6. finalize trx */
    W_DO(_pssm->commit_xct());

    // if we reached this point everything went ok
    trt.set_state(COMMITTED);

    /* 7. increase statistics */
    inc_no_cnt();

    return (RCOK);

} // EOF: NEW_ORDER



/****************************************************************** 
 *
 * class tpcc_loading_smt_t
 *
 ******************************************************************/

void tpcc_loading_smt_t::run()
{
    char fname[MAX_FILENAME_LEN];
    strcpy(fname, SHORE_TPCC_DATA_DIR);
    strcat(fname, "/");
    strcat(fname, _ptable->name());
    strcat(fname, ".dat");
    cout << _cnt << ". Loading " << _ptable->name() << endl;
    time_t ttablestart = time(NULL);
    w_rc_t e = _ptable->load_from_file(_pssm, fname);
    time_t ttablestop = time(NULL);
    if (e != RCOK) {
        cerr << _cnt << ". Error while loading " << _ptable->name() << " *****" << endl;
        _rv = 1;
    }
    else
        cout << _cnt << ". Done loading " << _ptable->name() << endl;

    cout << "Table " << _ptable->name() << " loaded in " << (ttablestop - ttablestart) << " secs..." << endl;
    _rv = 0;
}



/****************************************************************** 
 *
 * class tpcc_checking_smt_t
 *
 ******************************************************************/

void tpcc_checking_smt_t::run()
{
    cout << _cnt << ". Checking " << _ptable->name() << endl;
    if (!_ptable->check_all_indexes(_pssm))
        cerr << _cnt << ". Inconsistency in " << _ptable->name() << endl;
    else
        cout << _cnt << ". " << _ptable->name() << " OK..." << endl;
}


EXIT_NAMESPACE(tpcc);
