/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   tester_shore_env.cpp
 *
 *  @brief:  Implementation of the various test clients (Baseline, DORA, etc...)
 *
 *  @author: Ippokratis Pandis, July 2008
 */

#include "tests/common/tester_shore_client.h"

using namespace shore;


int _numOfWHs = DF_NUM_OF_WHS;



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
    // if BASELINE TPC-C MIX
    if (xct_type == XCT_MIX) {        
        xct_type = random_xct_type(rand(100));
    }
    
    switch (xct_type) {
        
        // TPC-C BASELINE
    case XCT_NEW_ORDER:
        W_DO(xct_new_order(xctid));  break;
    case XCT_PAYMENT:
        W_DO(xct_payment(xctid)); break;
    case XCT_ORDER_STATUS:
        W_DO(xct_order_status(xctid)); break;
    case XCT_DELIVERY:
        W_DO(xct_delivery(xctid)); break;
    case XCT_STOCK_LEVEL:
        W_DO(xct_stock_level(xctid)); break;

        // MBENCH BASELINE
    case XCT_MBENCH_WH:
        W_DO(xct_mbench_wh(xctid)); break;
    case XCT_MBENCH_CUST:
        W_DO(xct_mbench_cust(xctid)); break;

    default:
        assert (0); // UNKNOWN TRX-ID
    }
    return (RCOK);
}


/********************************************************************* 
 *
 *  Regular XCT functions
 *
 *********************************************************************/

w_rc_t baseline_tpcc_client_t::xct_new_order(int xctid) 
{ 
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    _tpccdb->run_new_order(xctid, atrt, _wh);    
    return (RCOK); 
}

w_rc_t baseline_tpcc_client_t::xct_payment(int xctid) 
{ 
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    _tpccdb->run_payment(xctid, atrt, _wh);    
    return (RCOK); 
}

w_rc_t baseline_tpcc_client_t::xct_order_status(int xctid) 
{ 
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    _tpccdb->run_order_status(xctid, atrt, _wh);    
    return (RCOK); 
}


w_rc_t baseline_tpcc_client_t::xct_delivery(int xctid) 
{ 
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    _tpccdb->run_delivery(xctid, atrt, _wh);
    return (RCOK); 
}


w_rc_t baseline_tpcc_client_t::xct_stock_level(int xctid) 
{ 
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    _tpccdb->run_stock_level(xctid, atrt, _wh);    
    return (RCOK); 
}



w_rc_t baseline_tpcc_client_t::xct_mbench_wh(int xctid) 
{ 
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    _tpccdb->run_mbench_wh(xctid, atrt, _wh);    
    return (RCOK); 
}

w_rc_t baseline_tpcc_client_t::xct_mbench_cust(int xctid) 
{ 
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    _tpccdb->run_mbench_cust(xctid, atrt, _wh);    
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
    
    switch (xct_type) {

        // TPC-C DORA
    case XCT_DORA_NEW_ORDER:
        W_DO(xct_dora_new_order(xctid));  break;
    case XCT_DORA_PAYMENT:
        W_DO(xct_dora_payment(xctid)); break;
    case XCT_DORA_ORDER_STATUS:
        W_DO(xct_dora_order_status(xctid)); break;
    case XCT_DORA_DELIVERY:
        W_DO(xct_dora_delivery(xctid)); break;
    case XCT_DORA_STOCK_LEVEL:
        W_DO(xct_dora_stock_level(xctid)); break;

        // MBENCH DORA
    case XCT_DORA_MBENCH_WH:
        W_DO(xct_dora_mbench_wh(xctid)); break;
    case XCT_DORA_MBENCH_CUST:
        W_DO(xct_dora_mbench_cust(xctid)); break;

    default:
        assert (0); // UNKNOWN TRX-ID
    }
    return (RCOK);
}


/********************************************************************* 
 *
 *  DORA XCT functions
 *
 *********************************************************************/

w_rc_t dora_tpcc_client_t::xct_dora_new_order(int xctid) 
{ 
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    _tpccdb->dora_new_order(xctid, atrt, _wh);    
    return (RCOK); 
}

w_rc_t dora_tpcc_client_t::xct_dora_payment(int xctid) 
{ 
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    _tpccdb->dora_payment(xctid, atrt, _wh);    
    return (RCOK); 
}

w_rc_t dora_tpcc_client_t::xct_dora_order_status(int xctid) 
{ 
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    _tpccdb->dora_order_status(xctid, atrt, _wh);    
    return (RCOK); 
}


w_rc_t dora_tpcc_client_t::xct_dora_delivery(int xctid) 
{ 
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    _tpccdb->dora_delivery(xctid, atrt, _wh);    
    return (RCOK); 
}


w_rc_t dora_tpcc_client_t::xct_dora_stock_level(int xctid) 
{ 
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    _tpccdb->dora_stock_level(xctid, atrt, _wh);    
    return (RCOK); 
}


w_rc_t dora_tpcc_client_t::xct_dora_mbench_wh(int xctid) 
{ 
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    _tpccdb->dora_mbench_wh(xctid, atrt, _wh);    
    return (RCOK); 
}

w_rc_t dora_tpcc_client_t::xct_dora_mbench_cust(int xctid) 
{ 
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    _tpccdb->dora_mbench_cust(xctid, atrt, _wh);    
    return (RCOK); 
}





