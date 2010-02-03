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

/** @file:   shore_tpcb_client.cpp
 *
 *  @brief:  Implementation of the test client for the TPCB benchmark
 *
 *  @author: Ippokratis Pandis, Feb 2009
 */

#include "workload/tpcb/shore_tpcb_client.h"

ENTER_NAMESPACE(tpcb);



/********************************************************************* 
 *
 *  baseline_tpcb_client_t
 *
 *********************************************************************/

const int 
baseline_tpcb_client_t::load_sup_xct(mapSupTrxs& stmap)
{
    // clears the supported trx map and loads its own
    stmap.clear();

    // Baseline TPC-B trxs
    stmap[XCT_TPCB_ACCT_UPDATE]    = "TPCB-AccountUpdate";

    return (stmap.size());
}


/********************************************************************* 
 *
 *  @fn:    run_one_xct
 *
 *  @brief: Baseline client - Entry point for running one trx 
 *
 *  @note:  The execution of this trx will not be stopped even if the
 *          measure interval has expired.
 *
 *********************************************************************/
 
w_rc_t 
baseline_tpcb_client_t::run_one_xct(int xct_type, int xctid) 
{
    // Set input
    trx_result_tuple_t atrt;
    bool bWake = false;
    if (_cp->take_one) {
        TRACE( TRACE_TRX_FLOW, "Sleeping\n");
        atrt.set_notify(_cp->wait+_cp->index);
        bWake = true;
    }

    // Pick a valid ID
    int selid = _selid;
    if (_selid==0) 
        selid = URand(0,_qf); 

    // Get one action from the trash stack
    assert (_tpcbdb);
    trx_request_t* arequest = new (_tpcbdb->_request_pool) trx_request_t;
    tid_t atid;
    arequest->set(NULL,atid,xctid,atrt,xct_type,selid);

    // Enqueue to worker thread
    assert (_worker);
    _worker->enqueue(arequest,bWake);
    return (RCOK);
}


EXIT_NAMESPACE(tpcb);


