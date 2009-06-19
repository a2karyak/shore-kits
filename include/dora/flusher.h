/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   flusher.h
 *
 *  @brief:  Specialization of a worker thread that acts as the dora-flusher.
 *
 *  @author: Ippokratis Pandis, Jun 2008
 */


/**
   Log flushing is a major source of ctxs (context switches). The higher the throughput
   of the system, the larger the number of ctxs due to log flushes, and the more 
   unnecessary work done due to those ctxs. In order to reduce the high rate of ctxs, 
   in DORA we break the final step (commit or abort) of the execution of each transaction 
   to two different phases: the work until the log-flush, and the rest. The commit() and
   abort() calls are broken to begin_commit()/end_commit() and begin_abort()/end_abort().
   The thread that it is responsible for the execution of the transaction, or in DORA's
   case the thread that executes the final-rvp, instead of having to ctx waiting for
   the log-flush to finish, it transfers the control to another specialized worker thread,
   called dora-flusher. 
   The dora-flusher, peaks all the transactions whose log-flush has finished, and they are
   runnable again, and finalizes the work, notifying the client etc... If there are no 
   other runnable trxs it calls to flush_all (flush until current gsn). The code looks 
   like:

   dora-worker that executes final-rvp:
   { ... begin_commit_xct();
   dora-flusher->enqueue_flushing(pxct); } 
   
   dora-flusher:
   while (true) {
   if (has_flushed()) {
      xct* pxct = flushed_queue->get_one();
      { ... pxct->finalize(); notify_client(); }
   if (has_flushing()) {
      move from flushing to flushed;} 
   { if (!has_flushed()) flush_all(); }

   In order to enable this mechanism Shore-kits needs to be configured with:
   --enable-dora --enable-dora-flusher
*/

#ifndef __DORA_FLUSHER_H
#define __DORA_FLUSHER_H


#include "dora.h"
#include "sm/shore/shore_worker.h"

using namespace shore;


ENTER_NAMESPACE(dora);


/******************************************************************** 
 *
 * @class: dora_flusher_t
 *
 * @brief: A template-based class for the worker threads
 * 
 ********************************************************************/

class dora_flusher_t : public base_worker_t
{   
public:
    typedef srmwqueue<rvp_t>    Queue;
    typedef Queue::ActionVecIt  QueueIterator;

private:

    guard<Queue> _flushing;
    guard<Queue> _flushed;

    guard<Pool> _pxct_flushing_pool;
    guard<Pool> _pxct_flushed_pool;

    const int _pre_STOP_impl() { return(0); }

    const int _work_ACTIVE_impl(); 

public:

    dora_flusher_t(ShoreEnv* env, c_str tname,
                  processorid_t aprsid = PBIND_NONE, const int use_sli = 0) 
        : base_worker_t(env, tname, aprsid, use_sli)
    { 
        int expected_sz = 50*60;

        _pxct_flushing_pool = new Pool(sizeof(xct_t*),expected_sz);
        _flushing = new Queue(_pxct_flushing_pool.get());
        assert (_flushing.get());
        _flushing->set(WS_INPUT_Q,this,2000,0);  // do 2000 loops before sleep

        _pxct_flushed_pool = new Pool(sizeof(xct_t*),expected_sz);
        _flushed = new Queue(_pxct_flushed_pool.get());        
        assert (_flushed.get());
        _flushed->set(WS_COMMIT_Q,this,0,0);        
    }

    ~dora_flusher_t() 
    { 
        _flushing.done();
        _pxct_flushing_pool.done();
        _flushed.done();
        _pxct_flushed_pool.done();
    }

    //// Access methods

    inline void enqueue_flushing(rvp_t* arvp) { _flushed->push(arvp, true); }
    

}; // EOF: dora_flusher_t


EXIT_NAMESPACE(dora);

#endif /** __DORA_FLUSHER_H */

