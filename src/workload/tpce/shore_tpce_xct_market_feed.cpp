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

/** @file:   shore_tpce_xct_market_feed.cpp
 *
 *  @brief:  Implementation of the Baseline Shore TPC-E MARKET FEED transction
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
 * TPC-E MARKET FEED
 *
 ********************************************************************/


w_rc_t ShoreTPCEEnv::xct_market_feed(const int xct_id, market_feed_input_t& pmfin)
{
    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

	  
    table_row_t* prlasttrade = _plast_trade_man->get_tuple(); //48
    assert (prlasttrade);

    table_row_t* prtradereq = _ptrade_request_man->get_tuple();
    assert (prtradereq);

    table_row_t* prtrade = _ptrade_man->get_tuple(); //149B
    assert (prtrade);

    table_row_t* prtradehist = _ptrade_history_man->get_tuple();
    assert (prtradehist);

    w_rc_t e = RCOK;
    rep_row_t areprow(_ptrade_man->ts());
    areprow.set(_ptrade_desc->maxsize());

    prlasttrade->_rep = &areprow;
    prtradereq->_rep = &areprow;
    prtrade->_rep = &areprow;
    prtradehist->_rep = &areprow;

    rep_row_t lowrep( _ptrade_man->ts());
    rep_row_t highrep( _ptrade_man->ts());

    // allocate space for the biggest of the table representations
    lowrep.set(_ptrade_desc->maxsize());
    highrep.set(_ptrade_desc->maxsize());
    {
	myTime 		now_dts;
	double 		req_price_quote;
	TIdent		req_trade_id;
	int			req_trade_qty;
	char		req_trade_type[4]; //3
	int 		rows_updated;

	now_dts = time(NULL);
	rows_updated = 0;

	for(int i = 0; i < max_feed_len; i++){  //the transaction should formally start here!
	    /**
	       update
	       LAST_TRADE
	       set
	       LT_PRICE = price_quote[i],
	       LT_VOL = LT_VOL + trade_qty[i],
	       LT_DTS = now_dts
	       where
	       LT_S_SYMB = symbol[i]
	    */

	    TRACE( TRACE_TRX_FLOW, "App: %d MF:lt-update (%s) (%d) (%d) (%ld) \n", xct_id, pmfin._symbol[i], pmfin._price_quote[i], pmfin._trade_qty[i], now_dts);
	    e = _plast_trade_man->lt_update_by_index(_pssm, prlasttrade, pmfin._symbol[i], pmfin._price_quote[i], pmfin._trade_qty[i], now_dts); //should we include lock_mode??
	    if (e.is_error()) {  goto done; }

	    rows_updated++; //there is only one row per symbol

	    /**
	       select
	       TR_T_ID,
	       TR_BID_PRICE,
	       TR_TT_ID,
	       TR_QTY
	       from
	       TRADE_REQUEST
	       where
	       TR_S_SYMB = symbol[i] and (
	       (TR_TT_ID = type_stop_loss and
	       TR_BID_PRICE >= price_quote[i]) or
	       (TR_TT_ID = type_limit_sell and
	       TR_BID_PRICE <= price_quote[i]) or
	       (TR_TT_ID = type_limit_buy and
	       TR_BID_PRICE >= price_quote[i])
	       )
	    */

	    //why doing scan?? create index on symbol!
	    /* trade_request_man_impl::table_iter* tr_iter;
	       TRACE( TRACE_TRX_FLOW, "App: %d MF:tr-get-table-iter \n", xct_id);
	       e = _ptrade_request_man->tr_get_table_iter(_pssm, tr_iter, prtradereq);
	       if (e.is_error()) { errors[0]++; goto done; }
	    */

	    /*
	      guard<index_scan_iter_impl<trade_request_t> > tr_iter;
	      {
	      index_scan_iter_impl<trade_request_t> *tmp_tr_iter;
	      TRACE( TRACE_TRX_FLOW, "App: %d MF:tr-get-iter-by-index5 \n", xct_id);
	      e = _ptrade_request_man->tr_get_iter_by_index5(_pssm, tmp_tr_iter, prtradereq, lowrep, highrep, pmfin._symbol[i], EX, true);
	      if (e.is_error()) { printf("MF- error1 \n"); goto done;}
	      tr_iter = tmp_tr_iter;
	      }


	      bool eof;
	      TRACE( TRACE_TRX_FLOW, "App: %d MF:tr-iter-next \n", xct_id);
	      e = tr_iter->next(_pssm, eof, *prtradereq);
	      if (e.is_error()) { printf("MF- error2 \n"); goto done; }
	      while(!eof){ 
	      printf("MF- not EOF \n");
	      char tr_s_symb[16], tr_tt_id[4]; //15,3
	      double tr_bid_price;

	      prtradereq->get_value(2, tr_s_symb, 16);
	      prtradereq->get_value(1, tr_tt_id, 4); //check padding in schema!
	      prtradereq->get_value(4, tr_bid_price);
	      assert(strcmp(tr_s_symb, pmfin._symbol[i]) == 0); 
		
	      if ((strcmp(tr_tt_id, pmfin._type_stop_loss) == 0 && (tr_bid_price >= pmfin._price_quote[i])) ||
	      (strcmp(tr_tt_id, pmfin._type_limit_sell) == 0 && (tr_bid_price <= pmfin._price_quote[i])) ||
	      (strcmp(tr_tt_id, pmfin._type_limit_buy)== 0 && (tr_bid_price >= pmfin._price_quote[i])))
	      {
	      printf("MF- MATCH \n");
	      prtradereq->get_value(0, req_trade_id);
	      prtradereq->get_value(4, req_price_quote);
	      prtradereq->get_value(1, req_trade_type, 4);
	      prtradereq->get_value(3, req_trade_qty);
	    */


	    ///////////////////

	    trade_request_man_impl::table_iter* tr_iter;
	    TRACE( TRACE_TRX_FLOW, "App: %d MF:tr-get-table-iter \n", xct_id);
	    e = _ptrade_request_man->tr_get_table_iter(_pssm, tr_iter, prtradereq);
	    if (e.is_error()) {  goto done; }

	    bool eof;
	    TRACE( TRACE_TRX_FLOW, "App: %d MF:tr-iter-next \n", xct_id);
	    e = tr_iter->next(_pssm, eof, *prtradereq);
	    if (e.is_error()) {  goto done; }
	    while(!eof){
		char tr_s_symb[16], tr_tt_id[4]; //15,3
		double tr_bid_price;

		prtradereq->get_value(2, tr_s_symb, 16);
		prtradereq->get_value(1, tr_tt_id, 4); //check padding in schema!
		prtradereq->get_value(4, tr_bid_price);

		if(strcmp(tr_s_symb, pmfin._symbol[i]) == 0 &&
		   (
		    (strcmp(tr_tt_id, pmfin._type_stop_loss) == 0 && (tr_bid_price >= pmfin._price_quote[i])) ||
		    (strcmp(tr_tt_id, pmfin._type_limit_sell) == 0 && (tr_bid_price <= pmfin._price_quote[i])) ||
		    (strcmp(tr_tt_id, pmfin._type_limit_buy)== 0 && (tr_bid_price >= pmfin._price_quote[i]))
		    ))
		    {
			prtradereq->get_value(0, req_trade_id);
			prtradereq->get_value(4, req_price_quote);
			prtradereq->get_value(1, req_trade_type, 4);
			prtradereq->get_value(3, req_trade_qty);

			//////////////////////////




			/**
			   update
			   TRADE
			   set
			   T_DTS   = now_dts,
			   T_ST_ID = status_submitted
			   where
			   T_ID = req_trade_id
			*/

			TRACE( TRACE_TRX_FLOW, "App: %d MF:t-update (%ld) (%ld) (%s) \n", xct_id, req_trade_id, now_dts, pmfin._status_submitted);
			e = _ptrade_man->t_update_dts_stdid_by_index(_pssm, prtrade, req_trade_id, now_dts, pmfin._status_submitted);
			if (e.is_error()) { goto done; }

			/**
			   delete
			   TRADE_REQUEST
			   where
			   current of request_list
			*/
			TRACE( TRACE_TRX_FLOW, "App: %d MF:tr-delete- \n", xct_id);
			e = _ptrade_request_man->delete_tuple(_pssm, prtradereq);
			if (e.is_error()) {  goto done; }
			prtradereq = _ptrade_request_man->get_tuple();
			assert (prtradereq);
			prtradereq->_rep = &areprow;

			/**
			   insert into
			   TRADE_HISTORY
			   values (
			   TH_T_ID = req_trade_id,
			   TH_DTS = now_dts,
			   TH_ST_ID = status_submitted)
			*/
			prtradehist->set_value(0, req_trade_id);
			prtradehist->set_value(1, now_dts);
			prtradehist->set_value(2, pmfin._status_submitted);

			TRACE( TRACE_TRX_FLOW, "App: %d MF:th-add-tuple (%ld) (%ld) (%s) \n", xct_id, req_trade_id, now_dts, pmfin._status_submitted);
			e = _ptrade_history_man->add_tuple(_pssm, prtradehist);
			if (e.is_error()) {goto done; }
		    }
		TRACE( TRACE_TRX_FLOW, "App: %d MF:tr-iter-next \n", xct_id);
		e = tr_iter->next(_pssm, eof, *prtradereq);
		if (e.is_error()) { goto done; }
	    }
	}
	assert(rows_updated == max_feed_len); //Harness Control
    }

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction
    // dumps the status of all the table rows used
    rlasttrade.print_tuple();
    rtradereq.print_tuple();
    rtrade.print_tuple();
    rtradehist.print_tuple();

#endif

 done:

#ifdef TESTING_TPCE           
    int exec=++trxs_cnt_executed[XCT_TPCE_MARKET_FEED - XCT_TPCE_MIX - 1];
    if(e.is_error()) trxs_cnt_failed[XCT_TPCE_MARKET_FEED - XCT_TPCE_MIX-1]++;
    if(exec%100==99) printf("MARKET_FEED executed: %d, failed: %d\n", exec, trxs_cnt_failed[XCT_TPCE_MARKET_FEED - XCT_TPCE_MIX-1]);
#endif





    // return the tuples to the cache
    _plast_trade_man->give_tuple(prlasttrade);
    _ptrade_request_man->give_tuple(prtradereq);
    _ptrade_man->give_tuple(prtrade);
    _ptrade_history_man->give_tuple(prtradehist);

    return (e);
}

EXIT_NAMESPACE(tpce);    
