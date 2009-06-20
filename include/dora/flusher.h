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
 * @struct: dora_flusher_stats_t
 *
 * @brief:  Various statistics for the dora-flusher
 * 
 ********************************************************************/

struct dora_flusher_stats_t 
{
    uint _finalized;
    uint _flushes;
    
    dora_flusher_stats_t() : _finalized(0), _flushes(0) { }
    ~dora_flusher_stats_t() { }

    void print() const;
    void reset();

}; // EOF: dora_flusher_stats_t



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

private:

    dora_flusher_stats_t _flusher_stats;

    guard<Queue> _flushing;
    guard<Queue> _flushed;

    guard<Pool> _pxct_flushing_pool;
    guard<Pool> _pxct_flushed_pool;

    const int _pre_STOP_impl() { return(0); }

    const int _work_ACTIVE_impl(); 

    void enqueue_flushed(rvp_t* arvp) { _flushed->push(arvp, true); }

public:

    dora_flusher_t(ShoreEnv* env, c_str tname,
                   processorid_t aprsid = PBIND_NONE, const int use_sli = 0);
    ~dora_flusher_t();

    inline void enqueue_flushing(rvp_t* arvp) { _flushing->push(arvp, true); }    

}; // EOF: dora_flusher_t


EXIT_NAMESPACE(dora);

#endif /** __DORA_FLUSHER_H */

