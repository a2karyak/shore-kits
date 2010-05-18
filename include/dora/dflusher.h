/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_flusher.h
 *
 *  @brief:  Specialization of a worker thread that acts as the dora-flusher.
 *
 *  @author: Ippokratis Pandis, Feb 2010
 */


/**
   Log flushing is a major source of context switches, as well as, of an 
   unexpected but definitely non-scalable problem with the notification of the 
   sleepers on cond vars, which makes almost impossible to saturate workloads 
   like TM1-UpdLocation with very high commit rate that put pressure on the log.

   In order to alleviate the problem with the high rate of context switches and 
   the overuse of condition variables in DORA, we implement a staged group
   commit mechanism. The thread that executes the final-rvp of each transaction 
   calls a lazy commit and does not context switch until the corresponding flush 
   completes. Instead it transfers the control to another specialized worker 
   thread, called dora-flusher. 

   The dora-flusher, monitors the statistics (how many transactions are in the 
   group unflushed, what is the size of the unflushed log, how much time it has 
   passed since the last flush) and makes a decision on when to flush. Typically, 
   if up to T msecs or N bytes or K xcts are unflushed a call to flush is 
   triggered.  

   The dora-flusher needs to do some short additional work per xct. It can either 
   notify back the worker thread that executed the corresponding final-rvp 
   that the flush had completed, and then to be responsibility of the worker thread 
   to do the xct clean-up (enqueue back to committed queues and notify client).
   Or it can give it to notifier thread to do the notifications. 
   The code looks like:

   dora-worker that executes final-rvp:
   {  ...
      commit_xct(lazy);
      dora-flusher->enqueue_toflush(pxct); } 

   
   dora-flusher:
   while (true) {
      rvp* prvp = toflush->dequeue();
      flushing->enqueue(rvp);
      if (time_to_flush()) {
         flush_all();
         while (!flushing->is_empty()) {
            prvp = flushing->dequeue();
            notifier->enqueue(prvp);
         }
      }
   }

   dora-notifier:
   while (true) {
      rvp* prvp = tonotify->dequeue();
      prvp->notify();
      prvp->notify_client();
   }

   In order to enable this mechanism Shore-kits needs to be configured with:
   --enable-dora --enable-dflusher
*/

#ifndef __DORA_FLUSHER_H
#define __DORA_FLUSHER_H

#include "sm/shore/shore_worker.h"
#include "sm/shore/shore_flusher.h"

#include "dora/rvp.h"

using namespace shore;

ENTER_NAMESPACE(dora);


class dora_notifier_t;

/******************************************************************** 
 *
 * @class: dora_flusher_t
 *
 * @brief: A class for the DFlusher that implements group commit (and
 *         batch flushing) in DORA
 * 
 ********************************************************************/

class dora_flusher_t : public base_worker_t
{   
public:
    typedef srmwqueue<terminal_rvp_t>    Queue;

private:

    flusher_stats_t _stats;

    guard<dora_notifier_t> _notifier;

    guard<Queue> _toflush;
    guard<Pool> _pxct_toflush_pool;

    guard<Queue> _flushing;
    guard<Pool> _pxct_flushing_pool;
    
    int _pre_STOP_impl();
    int _work_ACTIVE_impl(); 

public:

    dora_flusher_t(ShoreEnv* penv, c_str tname,
                   processorid_t aprsid = PBIND_NONE, 
                   const int use_sli = 0);
    ~dora_flusher_t();

    inline void enqueue_toflush(terminal_rvp_t* arvp) { _toflush->push(arvp,true); }

    int statistics();  

}; // EOF: dora_flusher_t



/******************************************************************** 
 *
 * @class: dora_notifier_t
 *
 * @brief: The notifier for flushed xcts. No need to enqueue them back
 *         to the worker of the final-rvp
 * 
 ********************************************************************/

class dora_notifier_t : public base_worker_t
{   
public:
    typedef srmwqueue<terminal_rvp_t>    Queue;

private:

    guard<Queue> _tonotify;
    guard<Pool> _pxct_tonotify_pool;
    
    int _pre_STOP_impl();
    int _work_ACTIVE_impl(); 

public:

    dora_notifier_t(ShoreEnv* env, c_str tname,
                    processorid_t aprsid = PBIND_NONE, 
                    const int use_sli = 0);
    ~dora_notifier_t();

    inline void enqueue_tonotify(terminal_rvp_t* arvp) { _tonotify->push(arvp,true); }

}; // EOF: dora_notifier_t


EXIT_NAMESPACE(dora);

#endif /** __DORA_FLUSHER_H */

