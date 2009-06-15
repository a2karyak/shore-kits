/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   logger.h
 *
 *  @brief:  The dora-logger.
 *
 *  @author: Ippokratis Pandis, Jun 2008
 */

#include "dora/logger.h"

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

const int dora_logger_t::_work_ACTIVE_impl()
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
    xct_t* pxct = NULL;

    // 1. check if signalled to stop
    while (get_control() == WC_ACTIVE) {        

        // reset the flags for the new loop
        pxct = NULL;
        set_ws(WS_LOOP);
        
        TRACE( TRACE_ALWAYS, "Looping\n");
        sleep(1);

        // committed actions

        // IP: TODO

        // THE COMMITTED ACTIONS BECOME FLUSHED XCT*
        // THE INPUT ACTIONS BECOME FLUSHING XCT*


//         // 2. first release any committed actions
//         while (_partition->has_committed()) {           

//             // 2a. get the first committed
//             apa = _partition->dequeue_commit();
//             assert (apa);
//             TRACE( TRACE_TRX_FLOW, "Received committed (%d)\n", apa->tid());
            
//             // 2b. release the locks acquired for this action
//             apa->trx_rel_locks(actionReadyList,actionPromotedList);
//             TRACE( TRACE_TRX_FLOW, "Received (%d) ready\n", actionReadyList.size());

//             // 2c. the action has done its cycle, and can be deleted
//             apa->giveback();
//             apa = NULL;

//             // 2d. serve any ready to execute actions 
//             //     (those actions became ready due to apa's lock releases)
//             for (BaseActionPtrIt it=actionReadyList.begin(); it!=actionReadyList.end(); ++it) {
//                 _serve_action(*it);
//                 ++_stats._served_waiting;
//             }

//             // clear the two lists
//             actionReadyList.clear();
//             actionPromotedList.clear();
//         }            

//         // new (input) actions

//         // 3. dequeue an action from the (main) input queue

//         // @note: it will spin inside the queue or (after a while) wait on a cond var

//         apa = _partition->dequeue();

//         // 4. check if it can execute the particular action
//         if (apa) {
//             TRACE( TRACE_TRX_FLOW, "Input trx (%d)\n", apa->tid());
//             if (apa->trx_acq_locks()) {
//                 // 4b. if it can acquire all the locks, 
//                 //     go ahead and serve this action
//                 _serve_action(apa);
//                 ++_stats._served_input;
//             }
//         }


    }

    TRACE( TRACE_ALWAYS, "Exiting\n");

    return (0);
}


EXIT_NAMESPACE(dora);
