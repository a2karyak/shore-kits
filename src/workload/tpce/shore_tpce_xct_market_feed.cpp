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

    w_rc_t e = RCOK;
	  
    table_row_t* prlasttrade = _plast_trade_man->get_tuple(); //48
    assert (prlasttrade);

    table_row_t* prtradereq = _ptrade_request_man->get_tuple();
    assert (prtradereq);

    table_row_t* prtrade = _ptrade_man->get_tuple(); //149B
    assert (prtrade);

    table_row_t* prtradehist = _ptrade_history_man->get_tuple();
    assert (prtradehist);

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
	if(pmfin._type_limit_buy[0] == '\n') {
	    e = RC(se_INVALID_INPUT);
	    goto done;
	}	

	myTime 		now_dts;
	double 		req_price_quote;
	TIdent		req_trade_id;
	int		req_trade_qty;
	char		req_trade_type[4]; //3
	int 		rows_updated;

	now_dts = time(NULL);

	//the transaction should formally start here!
	for(rows_updated = 0; rows_updated < max_feed_len; rows_updated++) {
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

	    TRACE( TRACE_TRX_FLOW, "App: %d MF:lt-update (%s) (%d) (%d) (%ld) \n",
		   xct_id, pmfin._symbol[rows_updated], pmfin._price_quote[rows_updated],
		   pmfin._trade_qty[rows_updated], now_dts);
	    e = _plast_trade_man->lt_update_by_index(_pssm, prlasttrade,
						     pmfin._symbol[rows_updated], pmfin._price_quote[rows_updated],
						     pmfin._trade_qty[rows_updated], now_dts);
	    if (e.is_error()) {  goto done; }

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
	    
	    // PIN: why doing scan?? create index on symbol!
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
		prtradereq->get_value(1, tr_tt_id, 4);
		prtradereq->get_value(4, tr_bid_price);

		if(strcmp(tr_s_symb, pmfin._symbol[rows_updated]) == 0 &&
		   (
		    (strcmp(tr_tt_id, pmfin._type_stop_loss) == 0 &&
		     (tr_bid_price >= pmfin._price_quote[rows_updated])) ||
		    (strcmp(tr_tt_id, pmfin._type_limit_sell) == 0 &&
		     (tr_bid_price <= pmfin._price_quote[rows_updated])) ||
		    (strcmp(tr_tt_id, pmfin._type_limit_buy)== 0 &&
		     (tr_bid_price >= pmfin._price_quote[rows_updated]))
		    )) {
		    prtradereq->get_value(0, req_trade_id);
		    prtradereq->get_value(4, req_price_quote);
		    prtradereq->get_value(1, req_trade_type, 4);
		    prtradereq->get_value(3, req_trade_qty);
		    
		    /**
		       update
		       TRADE
		       set
		       T_DTS   = now_dts,
		       T_ST_ID = status_submitted
		       where
		       T_ID = req_trade_id
		    */
		    
		    TRACE( TRACE_TRX_FLOW, "App: %d MF:t-update (%ld) (%ld) (%s) \n",
			   xct_id, req_trade_id, now_dts, pmfin._status_submitted);
		    e = _ptrade_man->t_update_dts_stdid_by_index(_pssm, prtrade, req_trade_id,
								 now_dts, pmfin._status_submitted);
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
		    
		    TRACE( TRACE_TRX_FLOW, "App: %d MF:th-add-tuple (%ld) (%ld) (%s) \n",
			   xct_id, req_trade_id, now_dts, pmfin._status_submitted);
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

    /* @note: PIN: the below is not executed because it ends up at an abstract virtual function
       //send triggered trades to the Market Exchange Emulator
       //via the SendToMarket interface.
       //This should be done after the related database changes have committed
       For (j=0; j<rows_sent; j++)
       {
         SendToMarketFromFrame(TradeRequestBuffer[i].symbol,
	 TradeRequestBuffer[i].trade_id,
	 TradeRequestBuffer[i].price_quote,
	 TradeRequestBuffer[i].trade_qty,
	 TradeRequestBuffer[i].trade_type);
       }
     */
    
#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction
    // dumps the status of all the table rows used
    rlasttrade.print_tuple();
    rtradereq.print_tuple();
    rtrade.print_tuple();
    rtradehist.print_tuple();
#endif

 done:
    // return the tuples to the cache
    _plast_trade_man->give_tuple(prlasttrade);
    _ptrade_request_man->give_tuple(prtradereq);
    _ptrade_man->give_tuple(prtrade);
    _ptrade_history_man->give_tuple(prtradehist);

    return (e);
}

EXIT_NAMESPACE(tpce);    
