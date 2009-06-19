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
    rvp_t* pop_rvp = NULL;

    QueueIterator flushing_i;

    TRACE( TRACE_ALWAYS, "Entering\n");
   
    // 1. check if signalled to stop
    while (get_control() == WC_ACTIVE) {        

        // reset the flags for the new loop
        prvp = NULL;
        set_ws(WS_LOOP);
        
        // 2. first clear any flushed actions
        while (!_flushed->is_empty()) {            
            prvp = _flushed->pop();
            TRACE( TRACE_ALWAYS, "Finalizing (%d)\n",
                   prvp->tid());
            smthread_t::me()->attach_xct(prvp->xct());
            _env->db()->end_commit_xct(prvp->_dummy_stats); 

#warning should the flusher do the dora-notification??

            int comActions = prvp->notify();
            prvp->giveback();
        }

        // 3. then move all flushing actions that have been flushed in
        //    the meantime to the flushed queue        
        gsn_t last_sync_gsn = _env->db()->last_sync_gsn();
        gsn_t my_last_gsn;
        while (!_flushing->is_empty()) {
            flushing_i = _flushing->_for_readers->begin();            
            if (flushing_i != _flushing->_for_readers->end()) {
                prvp = *(flushing_i++);
                my_last_gsn = _env->db()->my_last_gsn(prvp->xct());
                if ( my_last_gsn > last_sync_gsn ) {
                    // stop iterating on the first not flushed xct
                    TRACE( TRACE_ALWAYS, "Stop transfering (%d) (%d)\n", 
                           my_last_gsn, last_sync_gsn);
                    break;
                }
                else  {
                    // move to flushed
                    pop_rvp = _flushing->pop();
                    TRACE( TRACE_ALWAYS, "(%x) (%x)\n", prvp, pop_rvp);
                    //assert (*prvp==*pop_rvp);
                    _flushed->push(prvp,false);
                }
            }
            else break;
        }

        // 4. call to flush_all if no already flushed xct exist
        if (_flushed->is_empty()) _env->db()->flushlog();

    }

    TRACE( TRACE_ALWAYS, "Exiting\n");

    return (0);
}


EXIT_NAMESPACE(dora);
