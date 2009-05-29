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

/** @file:   shore_tpcb_client.h
 *
 *  @brief:  Defines test client for the TPC-B benchmark
 *
 *  @author: Ippokratis Pandis, Feb 2009
 */

#ifndef __SHORE_TPCB_CLIENT_H
#define __SHORE_TPCB_CLIENT_H


#include "workload/tpcb/shore_tpcb_env.h"

using namespace shore;


ENTER_NAMESPACE(tpcb);




/******************************************************************** 
 *
 * @enum:  baseline_tpcb_client_t
 *
 * @brief: The Baseline TPCB kit smthread-based test client class
 *
 ********************************************************************/

class baseline_tpcb_client_t : public base_client_t 
{
private:
    // workload parameters
    ShoreTPCBEnv* _tpcbdb;
    int _selid;
    trx_worker_t<ShoreTPCBEnv>* _worker;
    int _qf;
    

public:

    baseline_tpcb_client_t() { }     

    baseline_tpcb_client_t(c_str tname, const int id, ShoreTPCBEnv* env, 
                          const MeasurementType aType, const int trxid, 
                          const int numOfTrxs, 
                          processorid_t aprsid, const int selID, const int qf) 
	: base_client_t(tname,id,env,aType,trxid,numOfTrxs,aprsid),
          _tpcbdb(env), _selid(selID), _qf(qf)
    {
        assert (env);
        assert (_id>=0 && _qf>0);

        // pick worker thread
        _worker = _tpcbdb->tpcbworker(_id);
        TRACE( TRACE_DEBUG, "Picked worker (%s)\n", _worker->thread_name().data());
        assert (_worker);
    }

    ~baseline_tpcb_client_t() { }

    // every client class should implement this function
    static const int load_sup_xct(mapSupTrxs& map);

    // INTERFACE 

    w_rc_t run_one_xct(int xct_type, int xctid);    

}; // EOF: baseline_tpcb_client_t


EXIT_NAMESPACE(tpcb);


#endif /** __SHORE_TPCB_CLIENT_H */
