/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   test_shore_client.h
 *
 *  @brief:  Defines various test clients (Baseline, DORA, etc..)
 *
 *  @author: Ippokratis Pandis, July 2008
 */

#ifndef __TEST_SHORE_CLIENT_H
#define __TEST_SHORE_CLIENT_H

#include "stages/tpcc/common/tpcc_const.h"
#include "workload/tpcc/shore_tpcc_env.h"
#include "dora/tpcc/dora_tpcc.h"

using namespace tpcc;
using namespace shore;
using namespace dora;



// default database size (scaling factor)
const int DF_NUM_OF_WHS            = 10;
extern int _numOfWHs;

// Default values for the power-runs //

// default queried number of warehouses (queried factor)
const int DF_NUM_OF_QUERIED_WHS    = 10;

// default value to spread threads at WHs
const int DF_SPREAD_THREADS_TO_WHS = 1;

// default transaction id to be executed
const int DF_TRX_ID                = XCT_PAYMENT;

// Default values for the warmups //

// default number of queried WHs during warmup
const int DF_WARMUP_QUERIED_WHS = 10;



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
    tpcc_worker_t* _worker;

public:

    baseline_tpcc_client_t() { }     

    baseline_tpcc_client_t(c_str tname, const int id, ShoreTPCCEnv* env, 
                           MeasurementType aType, int trxid, int numOfTrxs, int useSLI,
                           processorid_t aprsid, int sWH) 
	: base_client_t(tname,id,env,aType,trxid,numOfTrxs,useSLI,aprsid),
          _tpccdb(env), _wh(sWH)
    {
        assert (env);
        assert (_wh>=0);

        // pick worker thread
        _worker = _tpccdb->tpccworker(_id);
        TRACE( TRACE_DEBUG, "Picked worker (%s)\n", _worker->thread_name().data());
        assert (_worker);
    }

    ~baseline_tpcc_client_t() { }

    // every client class should implement this functions
    static const int load_sup_xct(mapSupTrxs& map);

    // INTERFACE 

    w_rc_t run_one_xct(int xct_type, int xctid);    

}; // EOF: baseline_tpcc_client_t


/******************************************************************** 
 *
 * @enum:  dora_tpcc_client_t
 *
 * @brief: The Baseline TPC-C kit smthread-based test client class
 *
 ********************************************************************/

class dora_tpcc_client_t : public base_client_t 
{
private:
    // workload parameters
    DoraTPCCEnv* _tpccdb;    
    int _wh;

public:

    dora_tpcc_client_t() { }     

    dora_tpcc_client_t(c_str tname, const int id, DoraTPCCEnv* env, 
                       MeasurementType aType, int trxid, int numOfTrxs, int useSLI,
                       processorid_t aprsid, int sWH) 
	: base_client_t(tname,id,env,aType,trxid,numOfTrxs,useSLI,aprsid),
          _tpccdb(env), _wh(sWH)
    {
        assert (env);
        assert (_wh>=0);
    }

    ~dora_tpcc_client_t() { }

    // every client class should implement this functions
    static const int load_sup_xct(mapSupTrxs& map);

    // INTERFACE 

    w_rc_t run_one_xct(int xct_type, int xctid);    
    
}; // EOF: dora_tpcc_client_t


#endif /** __TEST_SHORE_CLIENT_H */
