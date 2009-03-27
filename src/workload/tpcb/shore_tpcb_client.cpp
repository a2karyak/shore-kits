/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   tester_tpcb_env.cpp
 *
 *  @brief:  Implementation of the various test clients (Baseline, DORA, etc...) for the TPCB benchmark
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





// /********************************************************************* 
//  *
//  *  dora_tpcb_client_t
//  *
//   *********************************************************************/

// const int dora_tpcb_client_t::load_sup_xct(mapSupTrxs& stmap)
// {
//     // clears the supported trx map and loads its own
//     stmap.clear();

//     // Baseline TPC-C trxs
//     stmap[XCT_DORA_MIX]          = "DORA-Mix";
//     stmap[XCT_DORA_NEW_ORDER]    = "DORA-NewOrder";
//     stmap[XCT_DORA_PAYMENT]      = "DORA-Payment";
//     stmap[XCT_DORA_ORDER_STATUS] = "DORA-OrderStatus";
//     stmap[XCT_DORA_DELIVERY]     = "DORA-Delivery";
//     stmap[XCT_DORA_STOCK_LEVEL]  = "DORA-StockLevel";

//     // Microbenchmarks
//     stmap[XCT_DORA_MBENCH_WH]    = "DORA-MBench-WHs";
//     stmap[XCT_DORA_MBENCH_CUST]  = "DORA-MBench-CUSTs";

//     return (stmap.size());
// }

// /********************************************************************* 
//  *
//  *  @fn:    run_one_xct
//  *
//  *  @brief: DORA client - Entry point for running one trx 
//  *
//  *  @note:  The execution of this trx will not be stopped even if the
//  *          measure internal has expired.
//  *
//  *********************************************************************/
 
// w_rc_t dora_tpcb_client_t::run_one_xct(int xct_type, int xctid) 
// {
//     // if DORA TPC-C MIX
//     if (xct_type == XCT_DORA_MIX) {        
//         xct_type = XCT_DORA_MIX + random_xct_type(rand(100));
//     }

//     // pick a valid wh id
//     int whid = _wh;
//     if (_wh==0) 
//         whid = URand(1,_qf); 

//     trx_result_tuple_t atrt;
//     if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    
//     switch (xct_type) {

//         // TPC-C DORA
//     case XCT_DORA_NEW_ORDER:
//         return (_tpcbdb->dora_new_order(xctid,atrt,whid));
//     case XCT_DORA_PAYMENT:
//         return (_tpcbdb->dora_payment(xctid,atrt,whid));
//     case XCT_DORA_ORDER_STATUS:
//         return (_tpcbdb->dora_order_status(xctid,atrt,whid));
//     case XCT_DORA_DELIVERY:
//         return (_tpcbdb->dora_delivery(xctid,atrt,whid));
//     case XCT_DORA_STOCK_LEVEL:
//         return (_tpcbdb->dora_stock_level(xctid,atrt,whid));

//         // MBENCH DORA
//     case XCT_DORA_MBENCH_WH:
//         return (_tpcbdb->dora_mbench_wh(xctid,atrt,whid));
//     case XCT_DORA_MBENCH_CUST:
//         return (_tpcbdb->dora_mbench_cust(xctid,atrt,whid));

//     default:
//         assert (0); // UNKNOWN TRX-ID
//     }
//     return (RCOK);
// }



EXIT_NAMESPACE(tpcb);


