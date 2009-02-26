/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tm1_client.h
 *
 *  @brief:  Defines various test clients (Baseline, DORA, etc..) for the TM1 benchmark
 *
 *  @author: Ippokratis Pandis, Feb 2009
 */

#ifndef __SHORE_TM1_CLIENT_H
#define __SHORE_TM1_CLIENT_H


#include "workload/tm1/shore_tm1_env.h"
#include "dora/tm1/dora_tm1.h"


ENTER_NAMESPACE(tm1);


using namespace shore;
using namespace dora;



/******************************************************************** 
 *
 * @enum:  baseline_tm1_client_t
 *
 * @brief: The Baseline TM1 kit smthread-based test client class
 *
 ********************************************************************/

class baseline_tm1_client_t : public base_client_t 
{
private:
    // workload parameters
    ShoreTM1Env* _tm1db;
    int _selid;
    trx_worker_t<ShoreTM1Env>* _worker;
    int _qf;
    
public:

    baseline_tm1_client_t() { }     

    baseline_tm1_client_t(c_str tname, const int id, ShoreTM1Env* env, 
                          const MeasurementType aType, const int trxid, 
                          const int numOfTrxs, 
                          processorid_t aprsid, const int selID, const int qf) 
	: base_client_t(tname,id,env,aType,trxid,numOfTrxs,aprsid),
          _tm1db(env), _selid(selID), _qf(qf)
    {
        assert (env);
        assert (_id>=0 && _qf>0);

        // pick worker thread
        _worker = _tm1db->tm1worker(_id);
        TRACE( TRACE_DEBUG, "Picked worker (%s)\n", _worker->thread_name().data());
        assert (_worker);
    }

    ~baseline_tm1_client_t() { }

    // every client class should implement this function
    static const int load_sup_xct(mapSupTrxs& map);

    // INTERFACE 

    w_rc_t run_one_xct(int xct_type, int xctid);    

}; // EOF: baseline_tm1_client_t



/******************************************************************** 
 *
 * @enum:  dora_tm1_client_t
 *
 * @brief: The Baseline TPC-C kit smthread-based test client class
 *
 ********************************************************************/

class dora_tm1_client_t : public base_client_t 
{
private:
    // workload parameters
    DoraTM1Env* _tm1db;    
    int _selid;
    int _qf;

public:

    dora_tm1_client_t() { }     

    dora_tm1_client_t(c_str tname, const int id, DoraTM1Env* env, 
                      const MeasurementType aType, const int trxid, 
                      const int numOfTrxs, 
                      processorid_t aprsid, const int selID, const int qf)  
	: base_client_t(tname,id,env,aType,trxid,numOfTrxs,aprsid),
          _tm1db(env), _selid(selID), _qf(qf)
    {
        assert (env);
        assert (_id>=0 && _qf>0);
    }

    ~dora_tm1_client_t() { }

    // every client class should implement this function
    static const int load_sup_xct(mapSupTrxs& map);

    // INTERFACE 

    w_rc_t run_one_xct(int xct_type, int xctid);    
    
}; // EOF: dora_tm1_client_t



EXIT_NAMESPACE(tm1);


#endif /** __SHORE_TM1_CLIENT_H */
