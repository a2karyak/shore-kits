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
    const int _serve_action(PartAction* paction);

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

    while (get_control() == WC_ACTIVE) {
        assert (_partition);
        
        // reset the flags for the new loop
        apa = NULL;
        bHadCommitted = 0; // an action had committed in this cycle
        set_ws(WS_LOOP);

        // 1. check if signalled to stop
        // propagate signal to next thread in the list, if any
        if (get_control() != WC_ACTIVE) {
            return (0);
        }


        // committed actions

        // 2. first release any committed actions
        while (_partition->has_committed()) {           
            //            assert (_partition->is_committed_owner(this));

            // 1a. get the first committed
            bHadCommitted = 1;
            apa = _partition->dequeue_commit();
            assert (apa);
            TRACE( TRACE_TRX_FLOW, "Received committed (%d)\n", apa->get_tid());
            //            assert (_partition==apa->get_partition());

            // 1b. release the locks acquired for this action
            apa->trx_rel_locks();

            // 1c. the action has done its cycle, and can be deleted
            apa->giveback();
            apa = NULL;
        }            


        // waiting actions

        // 3. if there were committed, go over the waiting list
        // Q: (ip) we may have a threshold value before we iterate the waiting list
        if (bHadCommitted && (_partition->has_waiting())) {

            // iterate over all actions and serve as many as possible
            apa = _partition->get_first_wait();
            assert (apa);
            TRACE( TRACE_TRX_FLOW, "First waiting (%d)\n", apa->get_tid());
            while (apa) {
                ++_stats._checked_waiting;
                if (apa->trx_acq_locks()) {
                    // if it can acquire all the locks, go ahead and serve 
                    // this action. then, remove it from the waiting list
                    _serve_action(apa);
                    _partition->remove_wait(); // the iterator should point to the previous element
                    ++_stats._served_waiting;
                }

                if (_partition->has_committed())
                    break;
                
                // loop over all waiting actions
                apa = _partition->get_next_wait();
                if (apa)
                    TRACE( TRACE_TRX_FLOW, "Next waiting (%d)\n", apa->get_tid());
            }            
        }


        // new (input) actions

        // 4. dequeue an action from the (main) input queue
        // it will spin inside the queue or (after a while) wait on a cond var
        //        assert (_partition->is_input_owner(this));
        if (_partition->has_committed())
            continue;
        apa = _partition->dequeue();

        // 5. check if it can execute the particular action
        if (apa) {
            TRACE( TRACE_TRX_FLOW, "Input trx (%d)\n", apa->get_tid());
            ++_stats._checked_input;
            if (!apa->trx_acq_locks()) {
                // 4a. if it cannot acquire all the locks, 
                //     enqueue this action to the waiting list
                _partition->enqueue_wait(apa);
            }
            else {
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
const int dora_worker_t<DataType>::_serve_action(PartAction* paction)
{
    // 1. get pointer to rvp
    rvp_t* aprvp = paction->get_rvp();
    assert (aprvp);

    // 2. attach to xct
    attach_xct(paction->get_xct());
    TRACE( TRACE_TRX_FLOW, "Attached to (%d)\n", paction->get_tid());
            
    // 3. serve action
    w_rc_t e = paction->trx_exec();
    if (e.is_error()) {
        TRACE( TRACE_ALWAYS, "Problem running xct (%d) [0x%x]\n",
               paction->get_tid(), e.err_num());
        ++_stats._problems;
        assert(0);
        return (de_WORKER_RUN_XCT);
    }          

    // 4. detach from trx
    TRACE( TRACE_TRX_FLOW, "Detaching from (%d)\n", paction->get_tid());
    detach_xct(paction->get_xct());

    // 5. finalize processing        
    if (aprvp->post()) {
        // last caller

        // execute the code of this rendez-vous point
        e = aprvp->run();            
        if (e.is_error()) {
            TRACE( TRACE_ALWAYS, "Problem running rvp for xct (%d) [0x%x]\n",
                   paction->get_tid(), e.err_num());
            assert(0);
            return (de_WORKER_RUN_RVP);

            // TODO (ip) instead of asserting it should either notify final rvp about failure,
            //           or abort
        }

        // enqueue committed actions
        int comActions = aprvp->notify();
//         TRACE( TRACE_TRX_FLOW, "(%d) actions committed for xct (%d)\n",
//                comActions, paction->get_tid());
            
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

