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


ENTER_NAMESPACE(shore);


const int NO_VALID_TRX_ID = -1;

/******************************************************************** 
 *
 * @enum  TrxState
 *
 * @brief Possible states of a transaction
 *
 ********************************************************************/

enum TrxState { UNDEF, UNSUBMITTED, SUBMITTED, POISSONED, 
		COMMITTED, ROLLBACKED };

static char* sTrxStates[6] = { "Undef", "Unsubmitted", "Submitted",
                               "Poissoned", "Committed", "Rollbacked" };


/** Helper functions */

/** @fn translate_state
 *  @brief Displays in a friendly way a TrxState
 */

inline char* translate_state(TrxState aState) {
    assert ((aState >= 0) && (aState < 6));
    return (sTrxStates[aState]);
}


/******************************************************************** 
 *
 * @class trx_result_tuple_t
 *
 * @brief Class used to represent the result of a transaction
 *
 ********************************************************************/

class trx_result_tuple_t 
{
private:

    TrxState R_STATE;
    int R_ID;
    condex* _notify;
   
public:

    trx_result_tuple_t() { reset(UNDEF, -1, NULL); }

    trx_result_tuple_t(TrxState aTrxState, int anID, condex* apcx = NULL) { 
        reset(aTrxState, anID, apcx);
    }

    ~trx_result_tuple_t() { }

    // @fn copy constructor
    trx_result_tuple_t(const trx_result_tuple_t& t) {
	reset(t.R_STATE, t.R_ID, t._notify);
    }      

    // @fn copy assingment
    trx_result_tuple_t& operator=(const trx_result_tuple_t& t) {        
        reset(t.R_STATE, t.R_ID, t._notify);        
        return (*this);
    }
    
    // @fn equality operator
    friend bool operator==(const trx_result_tuple_t& t, 
                           const trx_result_tuple_t& s) 
    {       
        return ((t.R_STATE == s.R_STATE) && (t.R_ID == s.R_ID));
    }


    // Access methods
    condex* get_notify() const { return (_notify); }
    void set_notify(condex* notify) { _notify = notify; }
    
    inline int get_id() const { return (R_ID); }
    inline void set_id(const int aID) { R_ID = aID; }

    inline TrxState get_state() { return (R_STATE); }
    inline char* say_state() { return (translate_state(R_STATE)); }
    inline void set_state(TrxState aState) { 
       assert ((aState >= UNDEF) && (aState <= ROLLBACKED));
       R_STATE = aState;
    }

    inline void reset(TrxState aTrxState, int anID, condex* notify) {
        // check for validity of inputs
        assert ((aTrxState >= UNDEF) && (aTrxState <= ROLLBACKED));
        assert (anID >= NO_VALID_TRX_ID);

        R_STATE = aTrxState;
        R_ID = anID;
	_notify = notify;
    }
        
}; // EOF: trx_result_tuple_t



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
        //assert (xct);
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
 * @note:  Template class SubShoreEnv should be a ShoreEnv-derived class
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
    int _work_ACTIVE_impl(); 

    int _pre_STOP_impl();

    // serves one action
    int _serve_action(Request* prequest);

public:

    trx_worker_t(ShoreEnv* env,SubShoreEnv* db,
                 c_str tname, 
                 processorid_t aprsid = PBIND_NONE,
                 const int use_sli = 0) 
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
    void enqueue(Request* arequest, const bool bWake=true);
    void init(const int lc) {
        _pqueue->set(WS_INPUT_Q,this,lc,0);
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
void 
trx_worker_t<SubShoreEnv>::enqueue(Request* arequest, const bool bWake)
{
    //assert (arequest);
    _pqueue->push(arequest,bWake);
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
int trx_worker_t<SubShoreEnv>::_work_ACTIVE_impl()
{    
    // bind to the specified processor
    _prs_id = PBIND_NONE;
    if (processor_bind(P_LWPID, P_MYID, _prs_id, NULL)) { //PBIND_NONE
        TRACE( TRACE_CPU_BINDING, "Cannot bind to processor (%d)\n", _prs_id);
        _is_bound = false;
    }
    else {
        TRACE( TRACE_CPU_BINDING, "Binded to processor (%d)\n", _prs_id);
        _is_bound = true;
    }

    w_rc_t e;
    Request* ar = NULL;

    // Check if signalled to stop
    while (get_control() == WC_ACTIVE) {
        
        // Reset the flags for the new loop
        ar = NULL;
        set_ws(WS_LOOP);

        // Dequeue a request from the (main) input queue
        // It will spin inside the queue or (after a while) wait on a cond var
        ar = _pqueue->pop();

        // Execute the particular request and deallocate it
        if (ar) {
            _serve_action(ar);
            ++_stats._served_input;
            _db->_request_pool.destroy(ar);
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
int trx_worker_t<SubShoreEnv>::_serve_action(Request* prequest)
{
    // 1. attach to xct
    assert (prequest);
    //smthread_t::me()->attach_xct(prequest->_xct);
    tid_t atid;
    w_rc_t e = _db->db()->begin_xct(atid);
    if (e.is_error()) {
        TRACE( TRACE_TRX_FLOW, "Problem beginning xct [0x%x]\n",
               e.err_num());
        ++_stats._problems;
        return (1);
    }          

    xct_t* pxct = smthread_t::me()->xct();
    assert (pxct);
    TRACE( TRACE_TRX_FLOW, "Begin (%d)\n", atid);
    prequest->_xct = pxct;
    prequest->_tid = atid;
            
    // 2. serve action
    e = _db->run_one_xct(prequest->_xct_id,
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



/****************************************************************** 
 *
 * @fn:     _pre_STOP_impl()
 *
 * @brief:  Goes over all the requests in the two queues and aborts 
 *          any unprocessed request
 * 
 ******************************************************************/

template <class SubShoreEnv>
int trx_worker_t<SubShoreEnv>::_pre_STOP_impl()
{
    Request* pr;
    int reqs_read  = 0;
    int reqs_write = 0;
    int reqs_abt   = 0;

    assert (_pqueue);

    // Go over the readers list
    for (; _pqueue->_read_pos != _pqueue->_for_readers->end(); _pqueue->_read_pos++) {
        pr = *(_pqueue->_read_pos);
        ++reqs_read;
        if (abort_one_trx(pr->_xct)) ++reqs_abt;
    }

    // Go over the writers list
    {
        CRITICAL_SECTION(q_cs, _pqueue->_lock);
        for (_pqueue->_read_pos = _pqueue->_for_writers->begin();
             _pqueue->_read_pos != _pqueue->_for_writers->end();
             _pqueue->_read_pos++) 
            {
                pr = *(_pqueue->_read_pos);
                ++reqs_write;
                if (abort_one_trx(pr->_xct)) ++reqs_abt;
            }
    }

    if ((reqs_read + reqs_write) > 0) {
        TRACE( TRACE_ALWAYS, "(%d) aborted before stopping. (%d) (%d)\n", 
               reqs_abt, reqs_read, reqs_write);
    }
    return (reqs_abt);
}
    


EXIT_NAMESPACE(shore);

#endif /** __SHORE_TRX_WORKER_H */

