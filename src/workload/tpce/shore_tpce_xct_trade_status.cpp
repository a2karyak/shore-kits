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

/** @file:   shore_tpce_xct_trade_status.cpp
 *
 *  @brief:  Implementation of the Baseline Shore TPC-E TRADE STATUS transaction
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

ENTER_NAMESPACE(tpce);

/******************************************************************** 
 *
 * TPC-E TRADE_STATUS
 *
 ********************************************************************/

w_rc_t ShoreTPCEEnv::xct_trade_status(const int xct_id, trade_status_input_t& ptsin)
{
    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    table_row_t* prbroker = _pbroker_man->get_tuple();
    assert (prbroker);

    table_row_t* prcustomer = _pcustomer_man->get_tuple(); //280
    assert (prcustomer);

    table_row_t* prcustacct = _pcustomer_account_man->get_tuple();
    assert (prcustacct);

    table_row_t* prexchange = _pexchange_man->get_tuple(); //291B
    assert (prexchange);

    table_row_t* prsecurity = _psecurity_man->get_tuple();
    assert (prsecurity);

    table_row_t* prstatustype = _pstatus_type_man->get_tuple();
    assert (prstatustype);

    table_row_t* prtrade= _ptrade_man->get_tuple();
    assert (prtrade);

    table_row_t* prtradetype = _ptrade_type_man->get_tuple();
    assert (prtradetype);

    w_rc_t e = RCOK;
    rep_row_t areprow(_ptrade_man->ts());
    areprow.set(_ptrade_desc->maxsize());

    prbroker->_rep = &areprow;
    prcustomer->_rep = &areprow;
    prcustacct->_rep = &areprow;
    prexchange->_rep = &areprow;
    prsecurity->_rep = &areprow;
    prstatustype->_rep = &areprow;
    prtrade->_rep = &areprow;
    prtradetype->_rep = &areprow;

    rep_row_t lowrep( _pexchange_man->ts());
    rep_row_t highrep( _pexchange_man->ts());

    // allocate space for the biggest of the table representations
    lowrep.set(_pexchange_desc->maxsize());
    highrep.set(_pexchange_desc->maxsize());

    {
	TIdent trade_id[50];
	myTime trade_dts[50];
	char status_name[50][11]; //10
	char type_name[50][13]; //12
	char symbol[50][16]; //15
	int trade_qty[50];
	char exec_name[50][50]; //49
	double charge[50];
	char s_name[50][71]; //70
	char ex_name[50][101]; //100

	/**
	   select first 50 row
	   trade_id[]    = T_ID,
	   trade_dts[]   = T_DTS,
	   status_name[] = ST_NAME,
	   type_name[]   = TT_NAME,
	   symbol[]      = T_S_SYMB,
	   trade_qty[]   = T_QTY,
	   exec_name[]   = T_EXEC_NAME,
	   charge[]      = T_CHRG,
	   s_name[]      = S_NAME,
	   ex_name[]     = EX_NAME
	   from
	   TRADE,
	   STATUS_TYPE,
	   TRADE_TYPE,
	   SECURITY,
	   EXCHANGE
	   where
	   T_CA_ID = acct_id and
	   ST_ID = T_ST_ID and
	   TT_ID = T_TT_ID and
	   S_SYMB = T_S_SYMB and
	   EX_ID = S_EX_ID
	   order by
	   T_DTS desc
	*/

	//descending order
	rep_row_t sortrep(_pexchange_man->ts());
	sortrep.set(_pexchange_desc->maxsize());
	desc_sort_buffer_t t_list(10);

	t_list.setup(0, SQL_LONG);
	t_list.setup(1, SQL_LONG);
	t_list.setup(2, SQL_FIXCHAR, 10);
	t_list.setup(3, SQL_FIXCHAR, 12);
	t_list.setup(4, SQL_FIXCHAR, 15);
	t_list.setup(5, SQL_INT);
	t_list.setup(6, SQL_FIXCHAR, 49);
	t_list.setup(7, SQL_FLOAT);
	t_list.setup(8, SQL_FIXCHAR, 70);
	t_list.setup(9, SQL_FIXCHAR, 100);

	table_row_t rsb(&t_list);
	desc_sort_man_impl t_sorter(&t_list, &sortrep);
	guard< index_scan_iter_impl<trade_t> > t_iter;
	{
	    index_scan_iter_impl<trade_t>* tmp_t_iter;
	    TRACE( TRACE_TRX_FLOW, "App: %d TS:t-iter-by-idx2 (%ld) (%ld) (%ld) \n", xct_id, ptsin._acct_id, 0, MAX_DTS);
	    e = _ptrade_man->t_get_iter_by_index2(_pssm, tmp_t_iter,
						  prtrade, lowrep, highrep,
						  ptsin._acct_id, 0, MAX_DTS);
	    if (e.is_error()) {  goto done; }
	    t_iter = tmp_t_iter;
	}
	bool eof;
	TRACE( TRACE_TRX_FLOW, "App: %d TS:t-iter-next \n", xct_id);
	e = t_iter->next(_pssm, eof, *prtrade);
	if (e.is_error()) {  goto done; }
	while(!eof){
	    myTime t_dts;
	    prtrade->get_value(1, t_dts);
	    rsb.set_value(0, t_dts);

	    TIdent t_id;
	    prtrade->get_value(0, t_id);
	    rsb.set_value(1, t_id);

	    char t_s_symbol[16]; //15
	    prtrade->get_value(5, t_s_symbol, 16);
	    rsb.set_value(4, t_s_symbol);

	    int t_qty;
	    prtrade->get_value(6, t_qty);
	    rsb.set_value(5, t_qty);

	    char t_exec_name[50]; //49
	    prtrade->get_value(9, t_exec_name, 50);
	    rsb.set_value(6, t_exec_name);

	    double t_chrg;
	    prtrade->get_value(11, t_chrg);
	    rsb.set_value(7, t_chrg);

	    char t_st_id[5], t_tt_id[4], t_s_symb[16]; //4, 3, 15

	    prtrade->get_value(2, t_st_id, 5);
	    prtrade->get_value(3, t_tt_id, 4);
	    prtrade->get_value(5, t_s_symb, 16);


	    TRACE( TRACE_TRX_FLOW, "App: %d TS:st-idx-probe (%s) \n", xct_id,  t_st_id);
	    e =  _pstatus_type_man->st_index_probe(_pssm, prstatustype, t_st_id);
	    if (e.is_error()) { goto done; }

	    char st_name[11]; //10
	    prstatustype->get_value(1, st_name, 11);
	    rsb.set_value(2, st_name);

	    
	    TRACE( TRACE_TRX_FLOW, "App: %d TS:tt-idx-probe (%s) \n", xct_id, t_tt_id);
	    e =  _ptrade_type_man->tt_index_probe(_pssm, prtradetype, t_tt_id);
	    if (e.is_error()) { goto done; }

	    char tt_name[13]; //12
	    prtradetype->get_value(1, tt_name, 13);
	    rsb.set_value(3, tt_name);


	    TRACE( TRACE_TRX_FLOW, "App: %d TS:s-idx-probe (%s) \n", xct_id, t_s_symb);
	    e =  _psecurity_man->s_index_probe(_pssm, prsecurity, t_s_symb);
	    if(e.is_error()) { goto done; }

	    char s_name[71]; //70
	    prsecurity->get_value(3, s_name, 71);
	    rsb.set_value(8, s_name);

	    char s_ex_id[7]; //6
	    prsecurity->get_value(4, s_ex_id, 7);


	    TRACE( TRACE_TRX_FLOW, "App: %d TS:ex-idx-probe (%s) \n", xct_id, s_ex_id);
	    e =  _pexchange_man->ex_index_probe(_pssm, prexchange, s_ex_id);
	    if(e.is_error()) { goto done; }

	    char ex_name[101]; //100
	    prexchange->get_value(1, ex_name, 101);
	    rsb.set_value(9, ex_name);


	    t_sorter.add_tuple(rsb);

	    TRACE( TRACE_TRX_FLOW, "App: %d TS:t-iter-next \n", xct_id);
	    e = t_iter->next(_pssm, eof, *prtrade);
	    if (e.is_error()) { goto done; }
	}
	assert (t_sorter.count());

	desc_sort_iter_impl t_list_sort_iter(_pssm, &t_list, &t_sorter);
	TRACE( TRACE_TRX_FLOW, "App: %d TS:t-sort-iter-next \n", xct_id);
	e = t_list_sort_iter.next(_pssm, eof, rsb);
	if (e.is_error()) {  goto done; }
	int i = 0;
	for(; i < max_trade_status_len && !eof; i++){
	    rsb.get_value(0, trade_dts[i]);
	    rsb.get_value(1, trade_id[i]);
	    rsb.get_value(2, status_name[i], 11);
	    rsb.get_value(3, type_name[i], 13);
	    rsb.get_value(4, symbol[i], 16);
	    rsb.get_value(5, trade_qty[i]);
	    rsb.get_value(6, exec_name[i], 50);
	    rsb.get_value(7, charge[i]);
	    rsb.get_value(8, s_name[i], 71);
	    rsb.get_value(9, ex_name[i], 101);

	    TRACE( TRACE_TRX_FLOW, "App: %d TS:t-sort-iter-next \n", xct_id);
	    e = t_list_sort_iter.next(_pssm, eof, rsb);
	    if (e.is_error()) {  goto done; }
	}
	assert(i == max_trade_status_len); 		
    
	/**
	   select
	   cust_l_name = C_L_NAME,
	   cust_f_name = C_F_NAME,
	   broker_name = B_NAME
	   from
	   CUSTOMER_ACCOUNT,
	   CUSTOMER,
	   BROKER
	   where
	   CA_ID = acct_id and
	   C_ID = CA_C_ID and
	   B_ID = CA_B_ID
	*/

	TRACE( TRACE_TRX_FLOW, "App: %d TS:ca-idx-probe (%ld) \n", xct_id,  ptsin._acct_id);
	e =  _pcustomer_account_man->ca_index_probe(_pssm, prcustacct, ptsin._acct_id);
	if (e.is_error()) { goto done; }

	TIdent ca_c_id, ca_b_id;

	prcustacct->get_value(2, ca_c_id);
	prcustacct->get_value(1, ca_b_id);

	TRACE( TRACE_TRX_FLOW, "App: %d TS:b-idx-probe (%ld) \n", xct_id, ca_b_id);
	e =  _pbroker_man->b_index_probe(_pssm, prbroker, ca_b_id);
	if (e.is_error()) { goto done; }

	TRACE( TRACE_TRX_FLOW, "App: %d TS:c-idx-probe (%ld) \n", xct_id,  ca_c_id);
	e =  _pcustomer_man->c_index_probe(_pssm, prcustomer, ca_c_id);
	if (e.is_error()) { goto done; }

	char cust_l_name[26]; //25
	char cust_f_name[21]; //20
	char broker_name[50]; //49

	prcustomer->get_value(3, cust_l_name, 26);
	prcustomer->get_value(4, cust_f_name, 21);
	prbroker->get_value(2, broker_name, 50);
    }

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction
    // dumps the status of all the table rows used
    rbroker.print_tuple();
    rcustacct.print_tuple();
    rcustomer.print_tuple();
    rexchange.print_tuple();
    rsecurity.print_tuple();
    rstatustype.print_tuple();
    rtrade.print_tuple();
    rtradetype.print_tuple();
#endif

 done:
    // return the tuples to the cache
    _pbroker_man->give_tuple(prbroker);
    _pcustomer_man->give_tuple(prcustomer);
    _pcustomer_account_man->give_tuple(prcustacct);
    _pexchange_man->give_tuple(prexchange);
    _psecurity_man->give_tuple(prsecurity);
    _pstatus_type_man->give_tuple(prstatustype);
    _ptrade_man->give_tuple(prtrade);
    _ptrade_type_man->give_tuple(prtradetype);

    return (e);
}

EXIT_NAMESPACE(tpce);    
