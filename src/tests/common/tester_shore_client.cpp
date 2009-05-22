/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-MT -- Multi-threaded port of the SHORE storage manager
   
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

/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   tester_shore_env.cpp
 *
 *  @brief:  Implementation of test client
 *
 *  @author: Ippokratis Pandis, July 2008
 */

#include "tests/common/tester_shore_client.h"

using namespace shore;


int _theSF = DF_SF;



/********************************************************************* 
 *
 *  baseline_tpcc_client_t
 *
 *********************************************************************/

const int baseline_tpcc_client_t::load_sup_xct(mapSupTrxs& stmap)
{
    // clears the supported trx map and loads its own
    stmap.clear();

    // Baseline TPC-C trxs
    stmap[XCT_MIX]          = "TPCC-Mix";
    stmap[XCT_NEW_ORDER]    = "TPCC-NewOrder";
    stmap[XCT_PAYMENT]      = "TPCC-Payment";
    stmap[XCT_ORDER_STATUS] = "TPCC-OrderStatus";
    stmap[XCT_DELIVERY]     = "TPCC-Delivery";
    stmap[XCT_STOCK_LEVEL]  = "TPCC-StockLevel";

    // Microbenchmarks
    stmap[XCT_MBENCH_WH]    = "TPCC-MBench-WHs";
    stmap[XCT_MBENCH_CUST]  = "TPCC-MBench-CUSTs";

    return (stmap.size());
}


/********************************************************************* 
 *
 *  @fn:    run_one_xct
 *
 *  @brief: Baseline client - Entry point for running one trx 
 *
 *  @note:  The execution of this trx will not be stopped even if the
 *          measure internal has expired.
 *
 *********************************************************************/
 
w_rc_t baseline_tpcc_client_t::run_one_xct(int xct_type, int xctid) 
{
    // 1. Initiate and detach from transaction
    tid_t atid;   
    W_DO(_tpccdb->db()->begin_xct(atid));
    xct_t* pxct = smthread_t::me()->xct();
    assert (pxct);
    TRACE( TRACE_TRX_FLOW, "Begin (%d)\n", atid);
    detach_xct(pxct);
    TRACE( TRACE_TRX_FLOW, "Detached from (%d)\n", atid);
    
    // 2. Set input
    trx_result_tuple_t atrt;
    bool bWake = false;
    if (_cp->take_one) {
        TRACE( TRACE_TRX_FLOW, "Sleeping on (%d)\n", atid);
        atrt.set_notify(_cp->wait+_cp->index);
        bWake = true;
    }
    // pick a valid wh id
    int whid = _wh;
    if (_wh==0) 
        whid = URand(1,_qf); 

    // 3. Get one action from the trash stack
    assert (_tpccdb);
    trx_request_t* arequest = new (_tpccdb->_request_pool) trx_request_t;
    arequest->set(pxct,atid,xctid,atrt,xct_type,whid);    

    // 4. enqueue to worker thread
    assert (_worker);
    _worker->enqueue(arequest,bWake);
    return (RCOK);
}



