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
 * @struct: worker_stats_t
 *
 * @brief:  Worker statistics
 * 
 * @note:   No lock-protected. Assumes that only the assigned worker thread
 *          will modify it.
 *
 ********************************************************************/

struct worker_stats_t
{
    int _processed;
    int _problems;

    int _checked_waiting;
    int _served_waiting;

    int _checked_input;
    int _served_input;
    
    int _condex_sleep;
    int _failed_sleep;


    worker_stats_t() 
        : _processed(0), _problems(0),
          _checked_waiting(0), _served_waiting(0),
          _checked_input(0), _served_input(0),
          _condex_sleep(0), _failed_sleep(0)
    { }

    ~worker_stats_t() { }
      
    void print_stats() const;

    void reset();

    void print_and_reset() { print_stats(); reset(); }

}; // EOF: worker_stats_t


/******************************************************************** 
 *
 * @class: base_worker_t
 *
 * @brief: An smthread-based non-template abstract base class for the 
 *         worker threads
 *
 * @note:  By default the worker thread is not bound to any processor.
 *         The creator of the worker thread (the partition) needs to
 *         decide where and if it will bind it somewhere.
 * @note:  Implemented as a state machine.
 * 
 ********************************************************************/

class base_worker_t : public thread_t 
{
protected:

    // status variables
    eWorkerControl volatile  _control;
    tatas_lock               _control_lock;
    eDataOwnerState volatile _data_owner;
    tatas_lock               _data_owner_lock;
    eWorkingState volatile   _ws;
    tatas_lock               _ws_lock;

    // cond var for sleeping instead of looping after a while
    condex                   _notify;

    // data
    ShoreEnv*                _env;    

    // needed for linked-list of workers
    base_worker_t*           _next;
    tatas_lock               _next_lock;

    // statistics
    worker_stats_t           _stats;

    // processor binding
    bool                     _is_bound;
    processorid_t            _prs_id;

    // states
    virtual const int _work_PAUSED_impl();
    virtual const int _work_ACTIVE_impl()=0;
    virtual const int _work_STOPPED_impl();

    void _print_stats_impl() const;

public:

    base_worker_t(ShoreEnv* env, c_str tname, processorid_t aprsid) 
        : thread_t(tname), 
          _env(env),
          _control(WC_PAUSED), _data_owner(DOS_UNDEF), _ws(WS_UNDEF),
          _next(NULL), _is_bound(false), _prs_id(aprsid)
    {
        assert (_env);
    }

    virtual ~base_worker_t() { 
        stats();
    }



    // access methods //

    // for the linked list
    void set_next(base_worker_t* apworker) {
        assert (apworker);
        CRITICAL_SECTION(next_cs, _next_lock);
        _next = apworker;
    }

    base_worker_t* get_next() { return (_next); }

    // data owner state
    void set_data_owner_state(const eDataOwnerState ados) {
        assert ((ados==DOS_ALONE)||(ados==DOS_MULTIPLE));
        CRITICAL_SECTION(dos_cs, _data_owner_lock);
        _data_owner = ados;
    }

    const bool is_alone_owner() { return (*&_data_owner==DOS_ALONE); }

    // working state
    inline void set_ws(const eWorkingState new_ws) {
        CRITICAL_SECTION(ws_cs, _ws_lock);
        eWorkingState old_ws = *&_ws;
        _ws=new_ws;
        if ((old_ws==WS_SLEEP)&&(new_ws!=WS_SLEEP)) {
            // wake up if sleeping
            membar_producer();
            condex_wakeup();
        }
    }

    inline const eWorkingState get_ws() { return (*&_ws); }

    inline const bool can_continue(const eWorkingState my_ws) {
        CRITICAL_SECTION(ws_cs, _ws_lock);
        return ((*&_ws==my_ws)||(*&_ws==WS_LOOP));
    }

    inline const bool is_sleeping(void) {
        return (*&_ws==WS_SLEEP);
    }
   
    // sleep-wake up
    inline const int condex_sleep() { 
        // can sleep only if in WS_LOOP
        // (if on WS_COMMIT_Q or WS_INPUT_Q mode means that a
        //  COMMIT or INPUT action were enqueued during this
        //  LOOP so there is no need to sleep).
        CRITICAL_SECTION(ws_cs, _ws_lock);
        if (*&_ws==WS_LOOP) {
            _ws=WS_SLEEP;
            ws_cs.exit();
            _notify.wait();
            ++_stats._condex_sleep;
            return (1);
        }
        ++_stats._failed_sleep;
        return (0); 
    }

    inline void condex_wakeup() { 
        assert (*&_ws!=WS_SLEEP); // the caller should have already changed it
        _notify.signal(); 
    }


    // working states //


    // thread control
    inline eWorkerControl get_control() { return (*&_control); }

    inline bool set_control(const eWorkerControl awc) {
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
    
    // commands
    inline void stop() {    
        set_control(WC_STOPPED);
        if (is_sleeping()) _notify.signal();
    }

    inline void start() {
        set_control(WC_ACTIVE);
        if (is_sleeping()) _notify.signal();
    }

    inline void pause() {
        set_control(WC_PAUSED);
        if (is_sleeping()) _notify.signal();
    }        

    // state implementation
    inline const int work_PAUSED()  { return (_work_PAUSED_impl());  }        
    inline const int work_ACTIVE()  { return (_work_ACTIVE_impl());  }
    inline const int work_STOPPED() { return (_work_STOPPED_impl()); }

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


    // helper //

    void stats() { _stats.print_stats(); }
    
}; // EOF: base_worker_t




/******************************************************************** 
 *
 * @class: worker_t
 *
 * @brief: A template-based class for the worker threads
 * 
 ********************************************************************/

template <class DataType>
class worker_t : public base_worker_t
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

    worker_t(ShoreEnv* env, Partition* apart, c_str tname,
             processorid_t aprsid = PBIND_NONE) 
        : base_worker_t(env, tname, aprsid),
          _partition(apart)
    { }

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

}; // EOF: worker_t



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
    //if (processor_bind(P_LWPID, P_MYID, _prs_id, NULL)) {
    if (processor_bind(P_LWPID, P_MYID, PBIND_NONE, NULL)) { // no-binding
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
const int worker_t<DataType>::_serve_action(PartAction* paction)
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
        TRACE( TRACE_TRX_FLOW, "(%d) actions committed for xct (%d)\n",
               comActions, paction->get_tid());
            
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

