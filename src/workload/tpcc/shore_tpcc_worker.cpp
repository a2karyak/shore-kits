/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:  shore_tpcc_worker.cpp
 *
 *  @brief: Implementation the worker threads in Baseline 
 *          (specialization of the Shore workers)
 *
 *  @author Ippokratis Pandis, Nov 2008
 */


#include "workload/tpcc/shore_tpcc_env.h"


ENTER_NAMESPACE(tpcc);


using namespace shore;


/******************************************************************** 
 *
 * @class: enqueue
 *
 * @brief: Enqueues a request to the queue of the specific worker thread
 * 
 ********************************************************************/

void tpcc_worker_t::enqueue(Request* arequest)
{
    assert (arequest);
    _queue.push(arequest);
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

extern void worker_thread_init();
extern void worker_thread_fini();

const int tpcc_worker_t::_work_ACTIVE_impl()
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
    worker_thread_init();
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
        ar = _queue.pop();

        // 3. execute the particular request
        if (ar) {
            _serve_action(ar);
            ++_stats._served_input;
        }
    }
    worker_thread_fini();
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

const int tpcc_worker_t::_serve_action(Request* prequest)
{
    // 1. attach to xct
    assert (prequest);
    smthread_t::me()->attach_xct(prequest->_xct);
    TRACE( TRACE_TRX_FLOW, "Attached to (%d)\n", prequest->_tid);
            
    // 2. serve action
    w_rc_t e = _tpccdb->run_one_xct(prequest->_xct_type,
                                    prequest->_xct_id,
                                    prequest->_whid,
                                    prequest->_result);
    if (e.is_error()) {
        TRACE( TRACE_TRX_FLOW, "Problem running xct (%d) [0x%x]\n",
               prequest->_tid, e.err_num());
        ++_stats._problems;
        //        assert(0);
        return (1);
    }          

    // 3. update worker stats
    ++_stats._processed;    
    return (0);
}



EXIT_NAMESPACE(tpcc);
