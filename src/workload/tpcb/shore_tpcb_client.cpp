/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
               Ecole Polytechnique Federale de Lausanne

                       Copyright (c) 2007-2008
                      Carnegie-Mellon University
   
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
    trx_result_tuple_t atrt;
    W_DO(_tpcbdb->db()->begin_xct());
    return _tpcbdb->run_one_xct(xctid, xct_type, _selid, atrt);
}


EXIT_NAMESPACE(tpcb);


