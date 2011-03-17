/* -*- mode:C++; c-basic-offset:4 -*-
   Shore-kits -- Benchmark implementations for Shore-MT

   Copyright (c) 2007-2009
   Data Intensive Applications and Systems Labaratory (DIAS)
   Ecole Polytechnique Federale de Lausanne

   All Rights Reserved.

   Permission to use, copy, modify and distribute this software and
   its documentation is hereby granted, provided that both the
   copyright notice and this permission notice appear in all copies of
   the software, derivative works or modified versions, and any
   portions thereof, and that both notices appear in supporting
   documentation.

   This code is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
   DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
   RESULTING FROM THE USE OF THIS SOFTWARE.
*/

/** @file:   shore_tpce_xct_customer_position.cpp
 *
 *  @brief:  Implementation of the Baseline Shore TPC-E CUSTOMER POSITION transaction
 *
 *  @author: Cansu Kaynak
 *  @author: Djordje Jevdjic
 */

#include "workload/tpce/shore_tpce_env.h"
#include "workload/tpce/tpce_const.h"
#include "workload/tpce/tpce_input.h"

#include <vector>
#include <numeric>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include "workload/tpce/egen/CE.h"
#include "workload/tpce/egen/TxnHarnessStructs.h"
#include "workload/tpce/shore_tpce_egen.h"

using namespace shore;
using namespace TPCE;

//#define TRACE_TRX_FLOW TRACE_ALWAYS
//#define TRACE_TRX_RESULT TRACE_ALWAYS

ENTER_NAMESPACE(tpce);

/******************************************************************** 
 *
 * TPC-E CUSTOMER POSITION
 *
 ********************************************************************/

w_rc_t ShoreTPCEEnv::xct_customer_position(const int xct_id, customer_position_input_t& pcpin)
{
    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    table_row_t* prcust = _pcustomer_man->get_tuple();	//280B
    assert (prcust);

    table_row_t* prcustacct = _pcustomer_account_man->get_tuple();
    assert (prcustacct);

    table_row_t* prholdsum = _pholding_summary_man->get_tuple();
    assert (prholdsum);

    table_row_t* prlasttrade = _plast_trade_man->get_tuple();
    assert (prlasttrade);

    table_row_t* prtradehist = _ptrade_history_man->get_tuple();
    assert (prtradehist); 

    table_row_t* prstatustype = _pstatus_type_man->get_tuple();
    assert (prstatustype);

    table_row_t* prtrade = _ptrade_man->get_tuple();	//149B
    assert (prtrade);

    w_rc_t e = RCOK;

    rep_row_t areprow(_pcustomer_man->ts());
    areprow.set(_pcustomer_desc->maxsize());

    prcust->_rep = &areprow;
    prcustacct->_rep = &areprow;
    prholdsum->_rep = &areprow;
    prlasttrade->_rep = &areprow;
    prtradehist->_rep = &areprow;
    prstatustype->_rep = &areprow;
    prtrade->_rep = &areprow;

    rep_row_t lowrep( _pcustomer_man->ts());
    rep_row_t highrep( _pcustomer_man->ts());

    // allocate space for the biggest of the table representations
    lowrep.set(_pcustomer_desc->maxsize());
    highrep.set(_pcustomer_desc->maxsize());

    {
	//pcpin.print(); //DELETE_ME
	//BEGIN FRAME 1
	TIdent acct_id[10];
	int acct_len;
	{
	    //printf("cust id: %ld ", pcpin._cust_id);
	    TIdent cust_id = pcpin._cust_id;
	    if(cust_id == 0){
		/**
		 *	select
		 *		cust_id = C_ID
		 *	from
		 *		CUSTOMER
		 *	where
		 *		C_TAX_ID = tax_id
		 */

		TRACE( TRACE_TRX_FLOW, "App: %d CP:c-idx4-probe (%s) \n", xct_id,  pcpin._tax_id);
		e =  _pcustomer_man->c_index4_probe(_pssm, prcust, pcpin._tax_id);
		if (e.is_error()) { goto done; }			  
		
		prcust->get_value(0, cust_id);
	    }
	    else{
		TRACE( TRACE_TRX_FLOW, "App: %d CP:c-idx-probe (%ld) \n", xct_id,  cust_id);
		e =  _pcustomer_man->c_index_probe(_pssm, prcust, cust_id);
		if (e.is_error()) { goto done; }			  
	    }

	    /**
	     *	select
	     *		c_st_id = C_ST_ID, //2
	     *		c_l_name = C_L_NAME,
	     *		c_f_name = C_F_NAME,
	     *		c_m_name = C_M_NAME,
	     *		c_gndr = C_GNDR,
	     *		c_tier = C_TIER,
	     *		c_dob = C_DOB,
	     *		c_ad_id = C_AD_ID,
	     *		c_ctry_1 = C_CTRY_1,
	     *		c_area_1 = C_AREA_1,
	     *		c_local_1 = C_LOCAL_1,
	     *		c_ext_1 = C_EXT_1,
	     *		c_ctry_2 = C_CTRY_2,
	     *		c_area_2 = C_AREA_2,
	     *		c_local_2 = C_LOCAL_2,
	     *		c_ext_2 = C_EXT_2,
	     *		c_ctry_3 = C_CTRY_3,
	     *		c_area_3 = C_AREA_3,
	     *		c_local_3 = C_LOCAL_3,
	     *		c_ext_3 = C_EXT_3,
	     *		c_email_1 = C_EMAIL_1,
	     *		c_email_2 = C_EMAIL_2
	     *	from
	     *		CUSTOMER
	     *	where
	     *		C_ID = cust_id
	     */

	    char c_st_id[5]; //4
	    prcust->get_value(2, c_st_id, 5);
	    char c_l_name[26]; //25
	    prcust->get_value(3, c_l_name, 26);
	    char c_f_name[21]; //20
	    prcust->get_value(4, c_f_name, 21);
	    char c_m_name[2]; //1
	    prcust->get_value(5, c_m_name, 2);
	    char c_gndr[2]; //1
	    prcust->get_value(6, c_gndr, 2);
	    short c_tier;
	    prcust->get_value(7, c_tier);
	    myTime c_dob;
	    prcust->get_value(8, c_dob);
	    TIdent c_ad_id;
	    prcust->get_value(9, c_ad_id);
	    char c_ctry_1[4]; //3
	    prcust->get_value(10, c_ctry_1, 4);
	    char c_area_1[4]; //3
	    prcust->get_value(11, c_area_1, 4);
	    char c_local_1[11]; //10
	    prcust->get_value(12, c_local_1, 11);
	    char c_ext_1[6]; //5
	    prcust->get_value(13, c_ext_1, 6);
	    char c_ctry_2[4]; //3
	    prcust->get_value(14, c_ctry_2, 4);
	    char c_area_2[4]; //3
	    prcust->get_value(15, c_area_2, 4);
	    char c_local_2[11]; //10
	    prcust->get_value(16, c_local_2, 11);
	    char c_ext_2[6]; //5
	    prcust->get_value(17, c_ext_2, 6);
	    char c_ctry_3[4]; //3
	    prcust->get_value(18, c_ctry_3, 4);
	    char c_area_3[4]; //3
	    prcust->get_value(19, c_area_3, 4);
	    char c_local_3[11]; //10
	    prcust->get_value(20, c_local_3, 11);
	    char c_ext_3[6]; //5
	    prcust->get_value(21, c_ext_3, 6);
	    char c_email_1[51]; //50
	    prcust->get_value(22, c_email_1, 51);
	    char c_email_2[51]; //50
	    prcust->get_value(23, c_email_2, 51);

	    /**
	     *	select first max_acct_len rows
	     *		acct_id[ ] = CA_ID,
	     *		cash_bal[ ] = CA_BAL,
	     *		assets_total[ ] = ifnull((sum(HS_QTY * LT_PRICE)),0)
	     *	from
	     *		CUSTOMER_ACCOUNT left outer join
	     *		HOLDING_SUMMARY on HS_CA_ID = CA_ID,
	     *		LAST_TRADE
	     *	where
	     *		CA_C_ID = cust_id and
	     *		LT_S_SYMB = HS_S_SYMB
	     *	group by
	     *		CA_ID, CA_BAL
	     *	order by
	     *		3 asc
	     */

	    guard< index_scan_iter_impl<customer_account_t> > ca_iter;
	    {
		index_scan_iter_impl<customer_account_t>* tmp_ca_iter;
		TRACE( TRACE_TRX_FLOW, "App: %d CP:ca-get-iter-by-idx2 (%ld) \n", xct_id,  cust_id);
		e = _pcustomer_account_man->ca_get_iter_by_index2(_pssm, tmp_ca_iter, prcustacct, lowrep, highrep, cust_id);
		if (e.is_error()) { goto done; }
		ca_iter = tmp_ca_iter;
	    }

	    //ascending order
	    rep_row_t sortrep(_pcustomer_man->ts());
	    sortrep.set(_pcustomer_desc->maxsize());
	    sort_buffer_t ca_list(3);

	    ca_list.setup(0, SQL_FLOAT);
	    ca_list.setup(1, SQL_FLOAT);
	    ca_list.setup(2, SQL_LONG);

	    table_row_t rsb(&ca_list);
	    sort_man_impl ca_sorter(&ca_list, &sortrep);

	    int i = 0;
	    bool eof;
	    TRACE( TRACE_TRX_FLOW, "App: %d CP:ca-iter-next \n", xct_id);
	    e = ca_iter->next(_pssm, eof, *prcustacct);
	    if (e.is_error()) { goto done; }
	    while(!eof){
		TIdent temp_id;
		double temp_balance = 0, temp_assets = 0;

		prcustacct->get_value(0, temp_id);
		prcustacct->get_value(5, temp_balance);

		guard< index_scan_iter_impl<holding_summary_t> > hs_iter;
		{
		    index_scan_iter_impl<holding_summary_t>* tmp_hs_iter;
		    TRACE( TRACE_TRX_FLOW, "App: %d CP:hs-get-iter-by-idx (%ld) \n", xct_id,  temp_id);
		    e = _pholding_summary_man->hs_get_iter_by_index(_pssm, tmp_hs_iter, prholdsum, lowrep, highrep, temp_id, true);
		    if (e.is_error()) { goto done; }
		    hs_iter = tmp_hs_iter;
		}

		TRACE( TRACE_TRX_FLOW, "App: %d CP:hs-iter-next  \n", xct_id);
		e = hs_iter->next(_pssm, eof, *prholdsum);
		if (e.is_error()) {  goto done; }
		while(!eof){
		    char symbol[16]; //15
		    prholdsum->get_value(1, symbol, 16);
		    int qty;
		    prholdsum->get_value(2, qty);

		    TRACE( TRACE_TRX_FLOW, "App: %d CP:lt-idx-probe (%s) \n", xct_id,  symbol);
		    e =  _plast_trade_man->lt_index_probe(_pssm, prlasttrade, symbol);
		    if (e.is_error()) { goto done; }

		    double lt_price = 0;
		    prlasttrade->get_value(2, lt_price);
		    temp_assets += (lt_price * qty);

		    //printf("LT_PRICE %4.2f QTY %d \n", lt_price, qty);

		    TRACE( TRACE_TRX_FLOW, "App: %d CP:hs-iter-next  \n", xct_id);
		    e = hs_iter->next(_pssm, eof, *prholdsum);
		    if (e.is_error()) { goto done; }
		}

		rsb.set_value(0, temp_assets);
		rsb.set_value(1, temp_balance);
		rsb.set_value(2, temp_id);

		TRACE( TRACE_TRX_FLOW, "App: %d CP:rsb add tuple  \n", xct_id);
		ca_sorter.add_tuple(rsb);

		TRACE( TRACE_TRX_FLOW, "App: %d CP:ca-iter-next \n", xct_id);
		e = ca_iter->next(_pssm, eof, *prcustacct);
		if (e.is_error()) { goto done; }
		i++;
	    }
	    assert (ca_sorter.count());
	    acct_len = i;

	    double cash_bal[10];
	    double assets_total[10];

	    sort_iter_impl ca_list_sort_iter(_pssm, &ca_list, &ca_sorter);
	    TRACE( TRACE_TRX_FLOW, "App: %d CP:ca-sorter-iter-next \n", xct_id);
	    e = ca_list_sort_iter.next(_pssm, eof, rsb);
	    if (e.is_error()) { goto done; }
	    for(int j = 0; j < 10 && !eof; j++) {
		rsb.get_value(2, acct_id[j]);
		rsb.get_value(1, cash_bal[j]);
		rsb.get_value(0, assets_total[j]);

		//printf("Acct_id %ld, cash_bal %4.2f assets_total %4.2f \n", acct_id[j], cash_bal[j], assets_total[j]); //DELETE_ME

		TRACE( TRACE_TRX_FLOW, "App: %d CP:ca-sorter-iter-next \n", xct_id);
		e = ca_list_sort_iter.next(_pssm, eof, rsb);
		if (e.is_error()) { goto done; }
	    }
	}
	//END OF FRAME 1
	assert(acct_len >= 1 && acct_len <= max_acct_len); //Harness control

	TIdent acctId = acct_id[pcpin._acct_id_idx];
	int hist_len;
	//BEGIN FRAME 2
	if(pcpin._get_history){
	    /**
	     *	select first 30 rows
	     *		trade_id[] = T_ID,
	     *		symbol[] = T_S_SYMB,
	     *		qty[] = T_QTY,
	     *		trade_status[] = ST_NAME,
	     *		hist_dts[] = TH_DTS
	     *	from
	     *		(select first 10 rows
	     *			T_ID as ID
	     *		from
	     *			TRADE
	     *		where
	     *			T_CA_ID = acct_id
	     *		order by
	     * 			T_DTS desc) as T,
	     *		TRADE,
	     *		TRADE_HISTORY,
	     *		STATUS_TYPE
	     *	where
	     *		T_ID = ID and
	     *		TH_T_ID = T_ID and
	     *		ST_ID = TH_ST_ID
	     *	order by
	     *		TH_DTS desc
	     */

	    TIdent trade_id[30];
	    char symbol[30][16]; //15
	    int qty[30];
	    char trade_status[30][11]; //10
	    myTime hist_dts[30];

	    TIdent id_list[10];
	    {
		guard<index_scan_iter_impl<trade_t> > t_iter;
		{
		    index_scan_iter_impl<trade_t>* tmp_t_iter;
		    TRACE( TRACE_TRX_FLOW, "App: %d CP:t-iter-by-idx2 %ld \n", xct_id, acctId);
		    e = _ptrade_man->t_get_iter_by_index2(_pssm, tmp_t_iter, prtrade, lowrep, highrep, acctId, 0, MAX_DTS);
		    t_iter = tmp_t_iter;
		    if (e.is_error()) { goto done; }
		}

		//descending order
		rep_row_t sortrep(_pcustomer_man->ts());
		sortrep.set(_pcustomer_desc->maxsize());

		desc_sort_buffer_t t_list(2);
		t_list.setup(0, SQL_LONG);
		t_list.setup(1, SQL_LONG);

		desc_sort_man_impl t_sorter(&t_list, &sortrep);
		table_row_t rsb(&t_list);

		bool eof;
		TRACE( TRACE_TRX_FLOW, "App: %d CP:t-iter-next \n", xct_id);
		e = t_iter->next(_pssm, eof, *prtrade);
		if (e.is_error()) { goto done; }
		while(!eof){
		    /* put the value into the sorted buffer */
		    TIdent temp_id;
		    myTime temp_dts;

		    prtrade->get_value(0, temp_id);
		    prtrade->get_value(1, temp_dts);

		    rsb.set_value(0, temp_dts);
		    rsb.set_value(1, temp_id);

		    t_sorter.add_tuple(rsb);

		    TRACE( TRACE_TRX_FLOW, "App: %d CP:t-iter-next \n", xct_id);
		    e = t_iter->next(_pssm, eof, *prtrade);
		    if (e.is_error()) { goto done; }
		}
		assert (t_sorter.count());

		desc_sort_iter_impl t_list_sort_iter(_pssm, &t_list, &t_sorter);
		TRACE( TRACE_TRX_FLOW, "App: %d CP:t-sorter-iter-next \n", xct_id);
		e = t_list_sort_iter.next(_pssm, eof, rsb);
		if (e.is_error()) { goto done; }
		for (int i = 0; i < 10 && !eof; i++) {
		    TIdent id;
		    rsb.get_value(1, id);

		    id_list[i] = id;
		  
		    //DELETE_ME
		    myTime dts;
		    rsb.get_value(0, dts);
		    //printf("DTS %ld id %ld \n", dts, id);

		    TRACE( TRACE_TRX_FLOW, "App: %d CP:t-sorter-iter-next \n", xct_id);
		    e = t_list_sort_iter.next(_pssm, eof, rsb);
		    if (e.is_error()) { goto done; }
		}
	    }

	    rep_row_t sortrep(_pcustomer_man->ts());
	    sortrep.set(_pcustomer_desc->maxsize());

	    desc_sort_buffer_t t_list(5);
	    t_list.setup(0, SQL_LONG); //th_dts
	    t_list.setup(1, SQL_LONG);
	    t_list.setup(2, SQL_FIXCHAR, 15);
	    t_list.setup(3, SQL_INT);
	    t_list.setup(4, SQL_FIXCHAR, 10);

	    desc_sort_man_impl t_sorter(&t_list, &sortrep);
	    table_row_t rsb(&t_list);

	    for (int i = 0; i < 10; i++) {
		TRACE( TRACE_TRX_FLOW, "App: %d CP:t-idx-probe (%ld) \n", xct_id,  id_list[i]);
		e =  _ptrade_man->t_index_probe(_pssm, prtrade, id_list[i]);
		if (e.is_error()) { goto done; }

		guard<index_scan_iter_impl<trade_history_t> > th_iter;
		{
		    index_scan_iter_impl<trade_history_t>* tmp_th_iter;
		    TRACE( TRACE_TRX_FLOW, "App: %d CP:th-iter-by-idx %ld \n", xct_id, id_list[i]);
		    e = _ptrade_history_man->th_get_iter_by_index(_pssm, tmp_th_iter, prtradehist, lowrep, highrep, id_list[i]);
		    if (e.is_error()) { goto done; }
		    th_iter = tmp_th_iter;
		}

		bool eof;
		e = th_iter->next(_pssm, eof, *prtradehist);
		if (e.is_error()) { goto done; }
		while(!eof){
		    char th_st_id[6]; //5
		    prtradehist->get_value(2, th_st_id, 6);

		    TRACE( TRACE_TRX_FLOW, "App: %d CP:st-idx-probe (%s) \n", xct_id,  th_st_id);
		    e =  _pstatus_type_man->st_index_probe(_pssm, prstatustype, th_st_id);
		    if (e.is_error()) { goto done; }

		    myTime th_dts;
		    prtradehist->get_value(1, th_dts);
		    rsb.set_value(0, th_dts);

		    rsb.set_value(1, id_list[i]);

		    char t_s_symb[16];
		    prtrade->get_value(5, t_s_symb, 16);
		    rsb.set_value(2, t_s_symb);

		    int t_qty;
		    prtrade->get_value(6, t_qty);
		    rsb.set_value(3, t_qty);

		    char st_name[11];
		    prstatustype->get_value(1, st_name, 11);
		    rsb.set_value(4, st_name);

		    t_sorter.add_tuple(rsb);

		    e = th_iter->next(_pssm, eof, *prtradehist);
		    if (e.is_error()) { goto done; }
		}
	    }
	    desc_sort_iter_impl t_list_sort_iter(_pssm, &t_list, &t_sorter);

	    bool eof;
	    e = t_list_sort_iter.next(_pssm, eof, rsb);
	    if (e.is_error()) { goto done; }
	    int i;
	    for(i = 0; i < 30 && !eof; i++){
		rsb.get_value(0, hist_dts[i]);
		rsb.get_value(1, trade_id[i]);
		rsb.get_value(2, symbol[i], 16);
		rsb.get_value(3, qty[i]);
		rsb.get_value(4, trade_status[i], 11);

		//DELETE_ME
		//printf("Hist-dts %ld, trade-id %ld, symbol %s, qty %d, trade_status %s \n", hist_dts[i], trade_id[i], symbol[i], qty[i], trade_status[i]);


		e = t_list_sort_iter.next(_pssm, eof, rsb);
		if (e.is_error()) { goto done; }
	    }
	    hist_len = i;			
	    assert(hist_len >= 10 && hist_len <= max_hist_len); //Harness control
	}
	//END OF FRAME 2
    }

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction
    // dumps the status of all the table rows used
    rcust.print_tuple();
    rcustacct.print_tuple();
    rholdsum.print_tuple();
    rlasttrade.print_tuple();
    rtradehist.print_tuple();
    rstatustype.print_tuple();
    rtrade.print_tuple();

#endif

 done:

#ifdef TESTING_TPCE           
    int exec=++trxs_cnt_executed[XCT_TPCE_CUSTOMER_POSITION - XCT_TPCE_MIX - 1];
    if(e.is_error()) trxs_cnt_failed[XCT_TPCE_CUSTOMER_POSITION - XCT_TPCE_MIX-1]++;
    if(exec%100==99) printf("CUSTOMER_POSITION executed: %d, failed: %d\n", exec, trxs_cnt_failed[XCT_TPCE_CUSTOMER_POSITION - XCT_TPCE_MIX-1]);
#endif

    // return the tuples to the cache
    _pcustomer_man->give_tuple(prcust);
    _pcustomer_account_man->give_tuple(prcustacct);
    _pholding_summary_man->give_tuple(prholdsum);
    _plast_trade_man->give_tuple(prlasttrade);
    _ptrade_history_man->give_tuple(prtradehist);
    _pstatus_type_man->give_tuple(prstatustype);
    _ptrade_man->give_tuple(prtrade);
    return (e);
}

EXIT_NAMESPACE(tpce);    
