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

/** @file:   tester_tm1_env.cpp
 *
 *  @brief:  Implementation of test client for the TM1 benchmark
 *
 *  @author: Ippokratis Pandis, Feb 2009
 */

#include "workload/tm1/shore_tm1_client.h"


ENTER_NAMESPACE(tm1);



/********************************************************************* 
 *
 *  baseline_tm1_client_t
 *
 *********************************************************************/

const int baseline_tm1_client_t::load_sup_xct(mapSupTrxs& stmap)
{
    // clears the supported trx map and loads its own
    stmap.clear();

    // Baseline TM1 trxs
    stmap[XCT_TM1_MIX]             = "TM1-Mix";
    stmap[XCT_TM1_GET_SUB_DATA]    = "TM1-GetSubData";
    stmap[XCT_TM1_GET_NEW_DEST]    = "TM1-GetNewDest";
    stmap[XCT_TM1_GET_ACC_DATA]    = "TM1-GetAccData";
    stmap[XCT_TM1_UPD_SUB_DATA]    = "TM1-UpdSubData";
    stmap[XCT_TM1_UPD_LOCATION]    = "TM1-UpdLocation";
    stmap[XCT_TM1_CALL_FWD_MIX]    = "TM1-CallFwd-Mix";
    stmap[XCT_TM1_INS_CALL_FWD]    = "TM1-InsCallFwd";
    stmap[XCT_TM1_DEL_CALL_FWD]    = "TM1-DelCallFwd";
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
 
w_rc_t baseline_tm1_client_t::run_one_xct(int xct_type, int xctid) 
{
    // 1. Initiate and detach from transaction
    tid_t atid;   
    W_DO(_tm1db->db()->begin_xct(atid));
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

    // pick a valid sf
    register int selsf = _selid;
    if (_selid==0) 
        selsf = URand(1,_qf); 

    // decide which ID inside that SF to use
    register int selid = (selsf-1)*TM1_SUBS_PER_SF + URand(0,TM1_SUBS_PER_SF-1);


    // 3. Get one action from the trash stack
    assert (_tm1db);
    trx_request_t* arequest = new (_tm1db->_request_pool) trx_request_t;
    arequest->set(pxct,atid,xctid,atrt,xct_type,selid);

    // 4. enqueue to worker thread
    assert (_worker);
    _worker->enqueue(arequest,bWake);
    return (RCOK);
}



EXIT_NAMESPACE(tm1);


