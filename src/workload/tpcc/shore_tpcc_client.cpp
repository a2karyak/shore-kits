/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
               Ecole Polytechnique Federale de Lausanne

                       Copyright (c) 2007-2008
                      Carnegie-Mellon University

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
/** @file:   shore_tpcc_client.cpp
 *
 *  @brief:  Implementation of the client for the TPCC benchmark
 *
 *  @author: Ippokratis Pandis, July 2008
 */

#include "workload/tpcc/shore_tpcc_client.h"

ENTER_NAMESPACE(tpcc);


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
    trx_result_tuple_t atrt;
    W_DO(_tpccdb->db()->begin_xct());
    return _tpccdb->run_one_xct(xctid, xct_type, _wh, atrt);
}



EXIT_NAMESPACE(tpcc);
