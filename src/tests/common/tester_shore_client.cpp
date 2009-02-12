/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   tester_shore_env.cpp
 *
 *  @brief:  Implementation of the various test clients (Baseline, DORA, etc...)
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
    stmap[XCT_MIX]          = "Mix";
    stmap[XCT_NEW_ORDER]    = "NewOrder";
    stmap[XCT_PAYMENT]      = "Payment";
    stmap[XCT_ORDER_STATUS] = "OrderStatus";
    stmap[XCT_DELIVERY]     = "Delivery";
    stmap[XCT_STOCK_LEVEL]  = "StockLevel";

    // Microbenchmarks
    stmap[XCT_MBENCH_WH]    = "MBench-WHs";
    stmap[XCT_MBENCH_CUST]  = "MBench-CUSTs";

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
    if (_cp->take_one) {
        TRACE( TRACE_TRX_FLOW, "Sleeping on (%d)\n", atid);
        atrt.set_notify(_cp->wait+_cp->index);
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
    _worker->enqueue(arequest);
    return (RCOK);
}





/********************************************************************* 
 *
 *  dora_tpcc_client_t
 *
  *********************************************************************/

const int dora_tpcc_client_t::load_sup_xct(mapSupTrxs& stmap)
{
    // clears the supported trx map and loads its own
    stmap.clear();

    // Baseline TPC-C trxs
    stmap[XCT_DORA_MIX]          = "DORA-Mix";
    stmap[XCT_DORA_NEW_ORDER]    = "DORA-NewOrder";
    stmap[XCT_DORA_PAYMENT]      = "DORA-Payment";
    stmap[XCT_DORA_ORDER_STATUS] = "DORA-OrderStatus";
    stmap[XCT_DORA_DELIVERY]     = "DORA-Delivery";
    stmap[XCT_DORA_STOCK_LEVEL]  = "DORA-StockLevel";

    // Microbenchmarks
    stmap[XCT_DORA_MBENCH_WH]    = "DORA-MBench-WHs";
    stmap[XCT_DORA_MBENCH_CUST]  = "DORA-MBench-CUSTs";

    return (stmap.size());
}

/********************************************************************* 
 *
 *  @fn:    run_one_xct
 *
 *  @brief: DORA client - Entry point for running one trx 
 *
 *  @note:  The execution of this trx will not be stopped even if the
 *          measure internal has expired.
 *
 *********************************************************************/
 
w_rc_t dora_tpcc_client_t::run_one_xct(int xct_type, int xctid) 
{
    // if DORA TPC-C MIX
    if (xct_type == XCT_DORA_MIX) {        
        xct_type = XCT_DORA_MIX + random_xct_type(rand(100));
    }

    // pick a valid wh id
    int whid = _wh;
    if (_wh==0) 
        whid = URand(1,_qf); 

    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    
    switch (xct_type) {

        // TPC-C DORA
    case XCT_DORA_NEW_ORDER:
        return (_tpccdb->dora_new_order(xctid,atrt,whid));
    case XCT_DORA_PAYMENT:
        return (_tpccdb->dora_payment(xctid,atrt,whid));
    case XCT_DORA_ORDER_STATUS:
        return (_tpccdb->dora_order_status(xctid,atrt,whid));
    case XCT_DORA_DELIVERY:
        return (_tpccdb->dora_delivery(xctid,atrt,whid));
    case XCT_DORA_STOCK_LEVEL:
        return (_tpccdb->dora_stock_level(xctid,atrt,whid));

        // MBENCH DORA
    case XCT_DORA_MBENCH_WH:
        return (_tpccdb->dora_mbench_wh(xctid,atrt,whid));
    case XCT_DORA_MBENCH_CUST:
        return (_tpccdb->dora_mbench_cust(xctid,atrt,whid));

    default:
        assert (0); // UNKNOWN TRX-ID
    }
    return (RCOK);
}





