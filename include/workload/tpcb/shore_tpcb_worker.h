/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:  shore_tpcc_worker.h
 *
 *  @brief: Wrapper for the worker threads in Baseline 
 *          (specialization of the Shore workers)
 *
 *  @author Ippokratis Pandis, Nov 2008
 */


#ifndef __SHORE_TPCC_WORKER_H
#define __SHORE_TPCC_WORKER_H


#include "sm/shore/shore_worker.h"
#include "core/trx_packet.h"


ENTER_NAMESPACE(tpcc);


using namespace qpipe;
using namespace shore;

class ShoreTPCCEnv;



/******************************************************************** 
 *
 * @struct: tpcc_action_t
 *
 * @brief:  Represents the requests in the Baseline system
 * 
 ********************************************************************/

struct tpcc_request_t
{
    // trx-specific
    xct_t*              _xct; // Not the owner
    tid_t               _tid;
    int                 _xct_id;
    trx_result_tuple_t  _result;
    int                 _xct_type;
    int                 _whid;

    tpcc_request_t() 
        : _xct(NULL),_xct_id(-1) 
    { }

    tpcc_request_t(xct_t* axct, tid_t atid, const int axctid,
                   trx_result_tuple_t& aresult, 
                   const int axcttype, const int awhid)
        : _xct(axct),_tid(atid),_xct_id(axctid),_result(aresult),
          _xct_type(axcttype), _whid(awhid)
    {
        assert (axct);
    }

    ~tpcc_request_t() { }  

    void set(xct_t* axct, tid_t atid, const int axctid,
             trx_result_tuple_t& aresult, const int axcttype, const int awhid)
    {
        assert (xct);
        _xct = axct;
        _tid = atid;
        _xct_id = axctid;
        _result = aresult;
        _xct_type = axcttype;
        _whid = awhid;
    }
};


/******************************************************************** 
 *
 * @class: tpcc_worker_t
 *
 * @brief: The baseline system worker threads
 * 
 ********************************************************************/

const int REQUESTS_PER_WORKER_POOL_SZ = 60;

class tpcc_worker_t : public base_worker_t
{
public:
    typedef tpcc_request_t     Request;
    typedef srmwqueue<Request> Queue;

private:

    ShoreTPCCEnv*  _tpccdb;
    guard<Queue>         _pqueue;
    guard<Pool>          _actionpool;

    // states
    const int _work_ACTIVE_impl(); 

    // serves one action
    const int _serve_action(Request* prequest);

public:

    tpcc_worker_t(ShoreEnv* env,ShoreTPCCEnv* tpcc, //ugly
                  c_str tname, 
                  processorid_t aprsid = PBIND_NONE, const int use_sli = 0) 
        : base_worker_t(env, tname, aprsid, use_sli), _tpccdb(tpcc)
    { 
        assert (env);
        _actionpool = new Pool(sizeof(Request*),REQUESTS_PER_WORKER_POOL_SZ);
        _pqueue = new Queue( _actionpool.get() );
    }
    ~tpcc_worker_t() { }

    // access methods
    void enqueue(Request* arequest);
    void init(const int lc) {
        _pqueue->set(WS_INPUT_Q,this,lc);
    }
        

}; // EOF: tpcc_worker_t



EXIT_NAMESPACE(tpcc);

#endif /** __SHORE_TPCC_WORKER_H */

