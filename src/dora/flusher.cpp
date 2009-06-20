/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   flusher.h
 *
 *  @brief:  The dora-flusher.
 *
 *  @author: Ippokratis Pandis, Jun 2008
 */

#include "dora/flusher.h"

ENTER_NAMESPACE(dora);


/****************************************************************** 
 *
 * @struct: dora_flusher_stats_t
 * 
 ******************************************************************/

void dora_flusher_stats_t::print() const 
{
    TRACE( TRACE_STATISTICS, "Xcts:       (%d)\n", _finalized);
    TRACE( TRACE_STATISTICS, "Flushes:    (%d)\n", _flushes);
    TRACE( TRACE_STATISTICS, "Xcts/Flush: (%.2f)\n",
           (double)_finalized/(double)_flushes);
}

void dora_flusher_stats_t::reset()
{
    _finalized = 0;
    _flushes = 0;
}


/******************************************************************** 
 *
 * @struct: dora_flusher_t
 * 
 ********************************************************************/

dora_flusher_t::dora_flusher_t(ShoreEnv* env, c_str tname,
                               processorid_t aprsid, 
                               const int use_sli) 
    : base_worker_t(env, tname, aprsid, use_sli)
{ 
    int expected_sz = 50*60;
    
    _pxct_flushing_pool = new Pool(sizeof(xct_t*),expected_sz);
    _flushing = new Queue(_pxct_flushing_pool.get());
    assert (_flushing.get());
    _flushing->set(WS_INPUT_Q,this,2000,0);  // do some (2000) loops before sleep
    
    _pxct_flushed_pool = new Pool(sizeof(xct_t*),expected_sz);
    _flushed = new Queue(_pxct_flushed_pool.get());        
    assert (_flushed.get());
    _flushed->set(WS_COMMIT_Q,this,0,0);        
}

dora_flusher_t::~dora_flusher_t() 
{ 
    // -- clear queues --
    // they better be empty by now
    assert (_flushing->is_empty());
    _flushing.done();
    _pxct_flushing_pool.done();
    assert (_flushed->is_empty());
    _flushed.done();
    _pxct_flushed_pool.done();
    
    // -- print statistics --
    _flusher_stats.print();
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

const int dora_flusher_t::_work_ACTIVE_impl()
{    
    //    TRACE( TRACE_DEBUG, "Activating...\n");
    int binding = envVar::instance()->getVarInt("dora-cpu-binding",0);
    if (binding==0) _prs_id = PBIND_NONE;

    // bind to the specified processor
    if (processor_bind(P_LWPID, P_MYID, _prs_id, NULL)) {
        //if (processor_bind(P_LWPID, P_MYID, PBIND_NONE, NULL)) { // no-binding
        TRACE( TRACE_CPU_BINDING, "Cannot bind to processor (%d)\n", _prs_id);
        _is_bound = false;
    }
    else {
        TRACE( TRACE_CPU_BINDING, "Binded to processor (%d)\n", _prs_id);
        _is_bound = true;
    }

    // state (WC_ACTIVE)

    // Start serving actions from the partition
    rvp_t* prvp = NULL;   
    gsn_t last_sync_gsn, my_last_gsn;

    // 1. check if signalled to stop
    while (get_control() == WC_ACTIVE) {        

        // reset the flags for the new loop
        prvp = NULL;
        set_ws(WS_LOOP);
        
        // 2. first clear any flushed xcts
        while (!_flushed->is_empty()) {            

            // 2a. attach to the head of the flushed queue and read gsns 
            prvp = _flushed->pop();
            my_last_gsn = _env->db()->my_last_gsn(prvp->xct());
            last_sync_gsn = _env->db()->last_sync_gsn();
            
            // 2b. if it has not been flushed already, call to flush all 
            //     before proceeding. This is the *only* xct in the flushed
            //     queue which has not been flushed already
            if ( my_last_gsn > last_sync_gsn ) {
                TRACE( TRACE_TRX_FLOW, "Flushing (%d) (%d)\n",
                       my_last_gsn, last_sync_gsn);
                _env->db()->flushlog();
                ++_flusher_stats._flushes;
            }
            
            // 2c. finalize xct
            smthread_t::me()->attach_xct(prvp->xct());
            _env->db()->end_commit_xct(prvp->_dummy_stats); 
            TRACE( TRACE_TRX_FLOW, "Finalized (%d)\n",
                   prvp->tid());

#warning should the flusher do the notifications (both dora and client)?
            int comActions = prvp->notify();
            prvp->notify_client();
            prvp->giveback();
        }

        // 3. then move all flushing actions that have been flushed in
        //    the meantime to the flushed queue. 
        //    
        // @note: It moves at least one not flushed xct to the flushed queue.
        //        Therefore, while finalizing xcts from the flushed queue it
        //        will bump into this not flushed xct and will call a flush-all().
        do {
            // 3a. get the head of the flushing queue and read gsns
            prvp = _flushing->pop();
            if (!prvp) break;
            my_last_gsn = _env->db()->my_last_gsn(prvp->xct());
            last_sync_gsn = _env->db()->last_sync_gsn();

            // 3b. in any case move it to flushed
            TRACE( TRACE_TRX_FLOW, "Moving (%d) (%d) (%d)\n", 
                   prvp->tid(), my_last_gsn, last_sync_gsn);
            enqueue_flushed(prvp);
            ++_flusher_stats._finalized;

            // 3c. stop iterating on the first not flushed xct
            if ( my_last_gsn > last_sync_gsn ) {
                TRACE( TRACE_TRX_FLOW, "Stop transfering (%d) (%d)\n", 
                       my_last_gsn, last_sync_gsn);
                break;
            }
        } while (!_flushing->is_empty());            
    }
    return (0);
}


EXIT_NAMESPACE(dora);
