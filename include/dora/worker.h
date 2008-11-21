/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:  worker.h
 *
 *  @brief: Wrapper for the worker threads in DORA 
 *          (specialization of the Shore workers)
 *
 *  @author Ippokratis Pandis, Sept 2008
 */


#ifndef __DORA_WORKER_H
#define __DORA_WORKER_H


#include "dora.h"
#include "sm/shore/shore_worker.h"


ENTER_NAMESPACE(dora);


using namespace shore;



/******************************************************************** 
 *
 * @class: dora_worker_t
 *
 * @brief: A template-based class for the worker threads
 * 
 ********************************************************************/

template <class DataType>
class dora_worker_t : public base_worker_t
{
public:

    typedef partition_t<DataType> Partition;
    typedef action_t<DataType>    PartAction;

private:
    
    Partition*     _partition;

    // states
    const int _work_ACTIVE_impl(); 

    // serves one action
    const int _serve_action(base_action_t* paction);

public:

    dora_worker_t(ShoreEnv* env, Partition* apart, c_str tname,
                  processorid_t aprsid = PBIND_NONE) 
        : base_worker_t(env, tname, aprsid),
          _partition(apart)
    { }

    ~dora_worker_t() { }


    // access methods

    // partition related
    void set_partition(Partition* apart) {
        assert (apart);
        _partition = apart;
    }

    Partition* get_partition() {
        return (_partition);
    }

}; // EOF: dora_worker_t



/****************************************************************** 
 *
 * @fn:     _work_ACTIVE_impl()
 *
 * @brief:  Implementation of the ACTIVE state
 *
 * @return: 0 on success
 * 
 ******************************************************************/

template <class DataType>
inline const int dora_worker_t<DataType>::_work_ACTIVE_impl()
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
    PartAction* apa = NULL;
    int bHadCommitted = 0;
    BaseActionPtrList actionReadyList;


    // 1. check if signalled to stop
    while (get_control() == WC_ACTIVE) {
        assert (_partition);
        
        // reset the flags for the new loop
        apa = NULL;
        bHadCommitted = 0; // an action had committed in this cycle
        set_ws(WS_LOOP);

        
        // committed actions

        // 2. first release any committed actions
        while (_partition->has_committed()) {           
            // 1a. get the first committed
            bHadCommitted = 1;
            apa = _partition->dequeue_commit();
            assert (apa);
            TRACE( TRACE_TRX_FLOW, "Received committed (%d)\n", apa->tid());
            
            // 1b. release the locks acquired for this action
            actionReadyList = apa->trx_rel_locks();
            TRACE( TRACE_TRX_FLOW, "Received (%d) ready\n", actionReadyList.size());

            // 1c. the action has done its cycle, and can be deleted
            apa->giveback();
            apa = NULL;

            // 1d. serve any ready to execute actions 
            //     (those actions became ready due to apa's lock releases)
            for (BaseActionPtrIt it=actionReadyList.begin(); it!=actionReadyList.end(); ++it) {
                _serve_action(*it);
                ++_stats._served_waiting;
            }
        }            


        // new (input) actions

        // 3. dequeue an action from the (main) input queue
        // it will spin inside the queue or (after a while) wait on a cond var
        if (_partition->has_committed())
            continue;
        apa = _partition->dequeue();

        // 5. check if it can execute the particular action
        if (apa) {
            TRACE( TRACE_TRX_FLOW, "Input trx (%d)\n", apa->tid());
            ++_stats._checked_input;

            if (apa->trx_acq_locks()) {
                // 4b. if it can acquire all the locks, 
                //     go ahead and serve this action
                _serve_action(apa);
                ++_stats._served_input;
            }
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

template <class DataType>
const int dora_worker_t<DataType>::_serve_action(base_action_t* paction)
{
    // 0. make sure that the action has all the keys it needs
    assert (paction);
    assert (paction->is_ready());

    // 1. get pointer to rvp
    rvp_t* aprvp = paction->rvp();
    assert (aprvp);

    // 2. attach to xct
    attach_xct(paction->xct());
    TRACE( TRACE_TRX_FLOW, "Attached to (%d)\n", paction->tid());
            
    // 3. serve action
    w_rc_t e = paction->trx_exec();
    if (e.is_error()) {
        TRACE( TRACE_ALWAYS, "Problem running xct (%d) [0x%x]\n",
               paction->tid(), e.err_num());
        ++_stats._problems;

        stringstream os;
        os << e << ends;
        string str = os.str();
        TRACE( TRACE_ALWAYS, "\n%s\n", str.c_str());

        return (de_WORKER_RUN_XCT);
    }          

    // 4. detach from trx
    TRACE( TRACE_TRX_FLOW, "Detaching from (%d)\n", paction->tid());
    detach_xct(paction->xct());

    // 5. finalize processing        
    if (aprvp->post()) {
        // last caller

        // execute the code of this rendez-vous point
        e = aprvp->run();            
        if (e.is_error()) {
            TRACE( TRACE_ALWAYS, "Problem running rvp for xct (%d) [0x%x]\n",
                   paction->tid(), e.err_num());
            return (de_WORKER_RUN_RVP);
        }

        // enqueue committed actions
        int comActions = aprvp->notify();
//         TRACE( TRACE_TRX_FLOW, "(%d) actions committed for xct (%d)\n",
//                comActions, paction->tid());
            
        // delete rvp
        aprvp->giveback();
        aprvp = NULL;
    }

    // 6. update worker stats
    ++_stats._processed;    
    return (0);
}



EXIT_NAMESPACE(dora);

#endif /** __DORA_WORKER_H */

