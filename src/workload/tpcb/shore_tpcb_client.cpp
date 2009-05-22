/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   tester_tpcb_env.cpp
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

const int baseline_tpcb_client_t::load_sup_xct(mapSupTrxs& stmap)
{
    // clears the supported trx map and loads its own
    stmap.clear();

    // Baseline TPC-B trxs
    stmap[XCT_TPCB_ACCT_UPDATE]    = "TPCB-AccountUpdate";
    stmap[XCT_TPCB_POPULATE_DB]    = "TPCB-PopulateDB";

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
 
w_rc_t baseline_tpcb_client_t::run_one_xct(int xct_type, int xctid) 
{
    // 1. Initiate and detach from transaction
    tid_t atid;   
    W_DO(_tpcbdb->db()->begin_xct(atid));
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

    // pick a valid ID
    int selid = _selid;
    if (_selid==0) 
        selid = URand(0,_qf); 

    // 3. Get one action from the trash stack
    assert (_tpcbdb);
    trx_request_t* arequest = new (_tpcbdb->_request_pool) trx_request_t;
    arequest->set(pxct,atid,xctid,atrt,xct_type,selid);

    // 4. enqueue to worker thread
    assert (_worker);
    _worker->enqueue(arequest,bWake);
    return (RCOK);
}


EXIT_NAMESPACE(tpcb);


