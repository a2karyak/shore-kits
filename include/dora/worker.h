/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file worker.h
 *
 *  @brief Wrapper for the worker threads in DORA
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#ifndef __DORA_WORKER_H
#define __DORA_WORKER_H

#include <cstdio>

// for binding LWP to cores
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>


#include "util.h"
#include "dora.h"


#include "sm/shore/shore_env.h"


ENTER_NAMESPACE(dora);


using namespace shore;


/******************************************************************** 
 *
 * @class: worker_t
 *
 * @brief: An smthread-based class for the worker threads
 *
 * @note:  By default the worker thread is not bound to any processor.
 *         The creator of the worker thread (the partition) needs to
 *         decide where and if it will bind it somewhere.
 * @note:  Implemented as a state machine.
 * 
 ********************************************************************/

template <class DataType>
class worker_t : public thread_t 
{
public:

    typedef partition_t<DataType> Partition;
    typedef action_t<DataType>    PartAction;

private:

    // status variables
    eWorkerControl volatile  _control;
    tatas_lock               _control_lock;
    eDataOwnerState volatile _data_owner;
    tatas_lock               _data_owner_lock;

    // cond var for sleeping instead of looping after a while
    WorkingState             _working;
    condex                   _notify;

    // data
    ShoreEnv*      _env;    
    Partition*     _partition;

    // needed for linked-list of workers
    worker_t<DataType>* _next;
    tatas_lock          _next_lock;

    // statistics
    int            _paused_wait;
    int            _processed;

    // processor binding
    bool           _is_bound;
    processorid_t  _prs_id;


    // states
    const int _work_PAUSED_impl();
    const int _work_ACTIVE_impl();
    const int _work_STOPPED_impl();

    void _print_stats_impl();

    // serves one action
    const int _serve_action(PartAction* paction);

public:

    worker_t(ShoreEnv* env, Partition* apart,             
             c_str tname,
             processorid_t aprsid = PBIND_NONE) 
        : thread_t(tname), 
          _env(env), _partition(apart),
          _control(WC_PAUSED), _data_owner(DOS_UNDEF),
          _next(NULL),
          _paused_wait(0), _processed(0),
          _is_bound(false), _prs_id(aprsid)
    {
        assert (_env);
    }

    ~worker_t() { }


    /** access methods */

    // partition related
    void set_partition(Partition* apart) {
        assert (apart);
        _partition = apart;
    }

    Partition* get_partition() {
        return (_partition);
    }

    // needed for setting the partition's queue
    eWorkerControl volatile* pwc() { return (&_control); }
    WorkingState* pws() { return (&_working); }
    condex* pcx() { return (&_notify); }


    // data owner state
    void set_data_owner_state(const eDataOwnerState ados) {
        assert ((ados==DOS_ALONE)||(ados==DOS_MULTIPLE));
        CRITICAL_SECTION(dos_cs, _data_owner_lock);
        _data_owner = ados;
    }

    const eDataOwnerState get_data_owner_state() {
        return (*&_data_owner);
    }

    // working state
    void set_working_state(const eWorkingState aws) {
        _working.set_state(aws);
    }

    const eWorkingState get_working_state() {
        return (_working.get_state());
    }


    // for the linked list
    void set_next(worker_t<DataType>* apworker) {
        assert (apworker);
        CRITICAL_SECTION(next_cs, _next_lock);
        _next = apworker;
    }


    // thread control
    bool set_control(const eWorkerControl awc) {
        //
        // Allowed transition matrix:
        //
        // |------------------------------------|
        // |(new)   | PAUSED | ACTIVE | STOPPED |
        // |(old)   |        |        |         |
        // |------------------------------------|
        // |PAUSED  |        |    Y   |    Y    |
        // |ACTIVE  |   Y    |        |    Y    |
        // |STOPPED |        |        |         |
        // |------------------------------------|
        //
        {
            CRITICAL_SECTION(wtcs, _control_lock);
            if ((*&_control == WC_PAUSED) && 
                ((awc == WC_ACTIVE) || (awc == WC_STOPPED))) {
                _control = awc;
                return (true);
            }

            if ((*&_control == WC_ACTIVE) && 
                ((awc == WC_PAUSED) || (awc == WC_STOPPED))) {
                _control = awc;
                return (true);
            }
        }
        TRACE( TRACE_DEBUG, "Not allowed transition (%d)-->(%d)\n",
               _control, awc);
        return (false);
    }

    inline eWorkerControl get_control() {
        return (*&_control);
    }

    
    // thread control commands
    inline void stop() {    
        set_control(WC_STOPPED);
        if (get_working_state() == WS_IDLE_CONDVAR)
            _notify.signal();
    }

    inline void start() {
        set_control(WC_ACTIVE);
    }

    inline void pause() {
        set_control(WC_PAUSED);
    }        

    inline void stats() {
        _print_stats_impl();
    }

    // state implementation
    inline const int work_PAUSED() {
        return (_work_PAUSED_impl());
    }
        
    inline const int work_ACTIVE() {
        return (_work_ACTIVE_impl());
    }

    inline const int work_STOPPED() {
        return (_work_STOPPED_impl());
    }


    // thread entrance
    inline void work() {
        // state machine
        while (true) {
            switch (get_control()) {
            case (WC_PAUSED):
                if (work_PAUSED())
                    return;
                break;

            case (WC_ACTIVE):
                if (work_ACTIVE())
                    return;
                break;

            case (WC_STOPPED): // exits
                work_STOPPED();
                return;
                break;

            default:
                assert(0); // should not be in any other state
            }
        }
    }

}; // EOF: worker_t



/****************************************************************** 
 *
 * @fn:     _work_PAUSED_impl()
 *
 * @brief:  Implementation of the PAUSED state
 *
 * @return: 0 on success
 * 
 ******************************************************************/

template <class DataType>
inline const int worker_t<DataType>::_work_PAUSED_impl()
{
    TRACE( TRACE_DEBUG, "Pausing...\n");

    // state (WC_PAUSED)

    // while paused loop and sleep
    while (get_control() == WC_PAUSED) {
        ++_paused_wait;
        sleep(1);
    }
    return (0);
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

template <class DataType>
inline const int worker_t<DataType>::_work_ACTIVE_impl()
{    
    TRACE( TRACE_DEBUG, "Activating...\n");

    // bind to the specified processor
    if (processor_bind(P_LWPID, P_MYID, _prs_id, NULL)) {
        TRACE( TRACE_ALWAYS, "Cannot bind to processor (%d)\n", _prs_id);
        _is_bound = false;
    }
    else {
        TRACE( TRACE_DEBUG, "Binded to processor (%d)\n", _prs_id);
        _is_bound = true;
    }

    // state (WC_ACTIVE)

    // Start serving actions from the partition
    w_rc_t e;
    PartAction* apa = NULL;
    int bHadCommitted = 0;

    while (get_control() == WC_ACTIVE) {
        assert (_partition);
        
        // reset the flags
        apa = NULL;
        bHadCommitted = 0; // an action had committed in this cycle


        // committed actions

        // 1. first release any committed actions
        while (_partition->has_committed()) {           
            assert (_partition->is_committed_owner(this));

            // 1a. get the first committed
            bHadCommitted = 1;
            apa = _partition->dequeue_commit();
            assert (apa);
            TRACE( TRACE_TRX_FLOW, "Committed trx (%d)\n", apa->get_tid());
            assert (_partition==apa->get_partition());

            // 1b. release the locks acquired for this action
            apa->trx_rel_locks();

            // 1c. the action has done its cycle, and can be deleted
            delete (apa);
            apa = NULL;
        }            


        // waiting actions

        // 2. if there were committed, go over the waiting list
        // Q: (ip) we may have a threshold value before we iterate the waiting list
        while (bHadCommitted && (_partition->has_waiting())) {

            // iterate over all actions and serve as many as possible
            apa = _partition->get_first_wait();
            assert (apa);
            while (apa) {
                TRACE( TRACE_TRX_FLOW, "Waiting trx (%d)\n", apa->get_tid());
                if (apa->trx_acq_locks()) {
                    // if it can acquire all the locks, go ahead and serve 
                    // this action. then, remove it from the waiting list
                    _serve_action(apa);
                    _partition->remove_wait();
                }
                
                // loop over all waiting actions
                apa = _partition->get_next_wait();
            }            
        }


        // new (input) actions

        // 3. dequeue an action from the (main) input queue
        // it will spin inside the queue or (after a while) wait on a cond var
        assert (_partition->is_input_owner(this));
        apa = _partition->dequeue();

        // if signalled to stop
        // propagate signal to next thread in the list, if any
        if (get_control() != WC_ACTIVE) {
            return (0);
        }

        // 4. check if it can execute the particular action
        if (apa) {
            TRACE( TRACE_TRX_FLOW, "Input trx (%d)\n", apa->get_tid());
            if (!apa->trx_acq_locks()) {
                // 4a. if it cannot acquire all the locks, 
                //     enqueue this action to the waiting list
                _partition->enqueue_wait(apa);
            }
            else {
                // 4b. if it can acquire all the locks, 
                //     go ahead and serve this action
                _serve_action(apa);
            }
        }
    }
    return (0);
}


/****************************************************************** 
 *
 * @fn:     _work_STOPPED_impl()
 *
 * @brief:  Implementation of the STOPPED state. 
 *          (1) It stops, joins and deletes the next thread(s) in the list
 *          (2) It clears any internal state
 *
 * @return: 0 on success
 * 
 ******************************************************************/

template <class DataType>
const int worker_t<DataType>::_work_STOPPED_impl()
{
    TRACE( TRACE_DEBUG, "Stopping...\n");

    // state (WC_STOPPED)

    // if signalled to stop
    // propagate signal to next thread in the list, if any
    CRITICAL_SECTION(next_cs, _next_lock);
    if (_next) { 
        // if someone is linked behind stop it and join it 
        _next->stop();
        _next->join();
        TRACE( TRACE_DEBUG, "Next joined...\n");
        delete (_next);
    }        
    _next = NULL; // join() ?

    // any cleanup code should be here

    // print statistics
    stats();

    return (0);
}


/****************************************************************** 
 *
 * @fn:     _print_stats_impl()
 *
 * @brief:  Printing stats
 * 
 ******************************************************************/

template <class DataType>
void worker_t<DataType>::_print_stats_impl()
{
    TRACE( TRACE_TRX_FLOW, "Processed (%d)\n", _processed);
    TRACE( TRACE_TRX_FLOW, "Waited (%d)\n", _paused_wait);
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
inline const int worker_t<DataType>::_serve_action(PartAction* paction)
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
            TRACE( TRACE_ALWAYS, "Problem runing rvp for xct (%d) [0x%x]\n",
                   paction->get_tid(), e.err_num());
            assert(0);
            return (de_WORKER_RUN_RVP);

            // TODO (ip) instead of asserting it should either notify final rvp about failure,
            //           or abort
        }

        // enqueue committed actions
        int comActions = aprvp->notify();
        TRACE( TRACE_TRX_FLOW, "(%d) actions committed for xct (%d)\n",
               comActions, paction->get_tid());
            
        // delete rvp
        delete (aprvp); 
        aprvp = NULL;
    }

    // 6. update worker stats
    ++_processed;    
}



EXIT_NAMESPACE(dora);

#endif /** __DORA_WORKER_H */

