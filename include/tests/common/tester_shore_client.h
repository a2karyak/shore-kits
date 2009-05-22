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

/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   test_shore_client.h
 *
 *  @brief:  Defines test client
 *
 *  @author: Ippokratis Pandis, July 2008
 */

#ifndef __TEST_SHORE_CLIENT_H
#define __TEST_SHORE_CLIENT_H

#include "workload/tpcc/tpcc_const.h"
#include "workload/tpcc/shore_tpcc_env.h"

using namespace tpcc;
using namespace shore;


// default database size (scaling factor)
const int DF_SF            = 10;
extern int _theSF;

// Default values for the power-runs //

// default queried factor
const int DF_NUM_OF_QUERIED_SF    = 10;

// default transaction id to be executed
const int DF_TRX_ID                = XCT_PAYMENT;

// Default values for the warmups //

// default number of queried SF during warmup
const int DF_WARMUP_QUERIED_SF = 10;



/******************************************************************** 
 *
 * @enum:  baseline_tpcc_client_t
 *
 * @brief: The Baseline TPC-C kit smthread-based test client class
 *
 ********************************************************************/

class baseline_tpcc_client_t : public base_client_t 
{
private:
    // workload parameters
    ShoreTPCCEnv* _tpccdb;    
    int _wh;
    trx_worker_t<ShoreTPCCEnv>* _worker;
    int _qf;
    

public:

    baseline_tpcc_client_t() { }     

    baseline_tpcc_client_t(c_str tname, const int id, ShoreTPCCEnv* env, 
                           const MeasurementType aType, const int trxid, 
                           const int numOfTrxs, 
                           processorid_t aprsid, const int sWH, const int qf) 
	: base_client_t(tname,id,env,aType,trxid,numOfTrxs,aprsid),
          _tpccdb(env), _wh(sWH), _qf(qf)
    {
        assert (env);
        assert (_wh>=0 && _qf>0);

        // pick worker thread
        _worker = _tpccdb->trxworker(_id);
        TRACE( TRACE_DEBUG, "Picked worker (%s)\n", _worker->thread_name().data());
        assert (_worker);
    }

    ~baseline_tpcc_client_t() { }

    // every client class should implement this function
    static const int load_sup_xct(mapSupTrxs& map);

    // INTERFACE 

    w_rc_t run_one_xct(int xct_type, int xctid);    

}; // EOF: baseline_tpcc_client_t


#endif /** __TEST_SHORE_CLIENT_H */
