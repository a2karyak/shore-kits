/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:  shore_trx_worker.h
 *
 *  @brief: Wrapper for the worker threads in Baseline 
 *          (specialization of the Shore workers)
 *
 *  @author Ippokratis Pandis, Nov 2008
 */


#ifndef __SHORE_TRX_WORKER_H
#define __SHORE_TRX_WORKER_H


#include "sm/shore/shore_worker.h"
#include "core/trx_packet.h"


ENTER_NAMESPACE(shore);


using namespace qpipe;



/******************************************************************** 
 *
 * @struct: trx_request_t
 *
 * @brief:  Represents the requests in the Baseline system
 * 
 ********************************************************************/

struct trx_request_t
{
    // trx-specific
    xct_t*              _xct; // Not the owner
    tid_t               _tid;
    int                 _xct_id;
    trx_result_tuple_t  _result;
    int                 _xct_type;
    int                 _spec_id; // used to be _whid

    trx_request_t() 
        : _xct(NULL),_xct_id(-1),_xct_type(-1),_spec_id(0)
    { }

    trx_request_t(xct_t* axct, tid_t atid, const int axctid,
                   trx_result_tuple_t& aresult, 
                   const int axcttype, const int aspecid)
        : _xct(axct),_tid(atid),_xct_id(axctid),_result(aresult),
          _xct_type(axcttype), _spec_id(aspecid)
    {
        assert (axct);
    }
    ~trx_request_t() { }  

    void set(xct_t* axct, tid_t atid, const int axctid,
             trx_result_tuple_t& aresult, const int axcttype, const int aspecid)
    {
        assert (xct);
        _xct = axct;
        _tid = atid;
        _xct_id = axctid;
        _result = aresult;
        _xct_type = axcttype;
        _spec_id = aspecid;
    }
};


/******************************************************************** 
 *
 * @class: trx_worker_t
 *
 * @brief: The baseline system worker threads
 *
 * @template class SubShoreEnv should be a ShoreEnv-derived class
 * 
 ********************************************************************/

const int REQUESTS_PER_WORKER_POOL_SZ = 60;

template <class SubShoreEnv>
class trx_worker_t : public base_worker_t
{
public:
    typedef trx_request_t     Request;
    typedef srmwqueue<Request> Queue;

private:

    SubShoreEnv*  _db;
    guard<Queue>         _pqueue;
    guard<Pool>          _actionpool;

    // states
    const int _work_ACTIVE_impl(); 

    // serves one action
    const int _serve_action(Request* prequest);

public:

    trx_worker_t(ShoreEnv* env,SubShoreEnv* db, //ugly
                 c_str tname, 
                 processorid_t aprsid = PBIND_NONE, const int use_sli = 0) 
        : base_worker_t(env, tname, aprsid, use_sli), _db(db)
    { 
        assert (env);
        _actionpool = new Pool(sizeof(Request*),REQUESTS_PER_WORKER_POOL_SZ);
        _pqueue = new Queue( _actionpool.get() );
    }
    ~trx_worker_t() 
    { 
        _pqueue = NULL;
        _actionpool = NULL;
    }

    // access methods
    void enqueue(Request* arequest);
    void init(const int lc) {
        _pqueue->set(WS_INPUT_Q,this,lc);
    }
        

}; // EOF: trx_worker_t



/******************************************************************** 
 *
 * @class: enqueue
 *
 * @brief: Enqueues a request to the queue of the specific worker thread
 * 
 ********************************************************************/

template <class SubShoreEnv>
void trx_worker_t<SubShoreEnv>::enqueue(Request* arequest)
{
    assert (arequest);
    _pqueue->push(arequest);
}



/****************************************************************** 
 *
 * @fn:     _work_ACTIVE_impl()
 *
 * @brief:  Implementation of the ACTIVE state
 *
 * @return: 0 on success
 * 
 ******************************************************************/

template <class SubShoreEnv>
const int trx_worker_t<SubShoreEnv>::_work_ACTIVE_impl()
{    
    //    TRACE( TRACE_DEBUG, "Activating...\n");

    // bind to the specified processor
    //if (processor_bind(P_LWPID, P_MYID, _prs_id, NULL)) {
    if (processor_bind(P_LWPID, P_MYID, PBIND_NONE, NULL)) { // no-binding
        TRACE( TRACE_CPU_BINDING, "Cannot bind to processor (%d)\n", _prs_id);
        _is_bound = false;
    }
    else {
        TRACE( TRACE_CPU_BINDING, "Binded to processor (%d)\n", _prs_id);
        _is_bound = true;
    }

    // state (WC_ACTIVE)

    // Start serving actions from the partition
    w_rc_t e;
    Request* ar = NULL;

    // 1. check if signalled to stop
    while (get_control() == WC_ACTIVE) {
        
        // reset the flags for the new loop
        ar = NULL;
        set_ws(WS_LOOP);

        // new (input) actions

        // 2. dequeue a request from the (main) input queue
        // it will spin inside the queue or (after a while) wait on a cond var
        ar = _pqueue->pop();

        // 3. execute the particular request
        if (ar) {
            _serve_action(ar);
            ++_stats._served_input;
        }
    }
    return (0);
}



/****************************************************************** 
 *
 * @fn:     _serve_action()
 *
 * @brief:  Executes an action, once this action is cleared to execute.
 *          That is, it assumes that the action has already acquired 
 *          all the required locks from its partition.
 * 
 ******************************************************************/

template <class SubShoreEnv>
const int trx_worker_t<SubShoreEnv>::_serve_action(Request* prequest)
{
    // 1. attach to xct
    assert (prequest);
    smthread_t::me()->attach_xct(prequest->_xct);
    TRACE( TRACE_TRX_FLOW, "Attached to (%d)\n", prequest->_tid);
            
    // 2. serve action
    w_rc_t e = _db->run_one_xct(prequest->_xct_id,
                                prequest->_xct_type,
                                prequest->_spec_id,
                                prequest->_result);
    if (e.is_error()) {
        TRACE( TRACE_TRX_FLOW, "Problem running xct (%d) (%d) [0x%x]\n",
               prequest->_tid, prequest->_xct_id, e.err_num());
        ++_stats._problems;
        return (1);
    }          

    // 3. update worker stats
    ++_stats._processed;    
    return (0);
}




EXIT_NAMESPACE(shore);

#endif /** __SHORE_TRX_WORKER_H */

