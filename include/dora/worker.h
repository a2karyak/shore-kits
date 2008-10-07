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
 * @enum:  WorkingState
 *
 * @brief: Possible states while working on a task
 *
 * @note:  Finished = Done assigned job but still trx not decided
 *         (Finished != Idle)
 *
 ********************************************************************/

enum WorkingState { WS_UNDEF, WS_IDLE, WS_ASSIGNED, WS_FINISHED };


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

    typedef partition_t<DataType> partition;
    typedef action_t<DataType>    part_action;

private:

    // status
    WorkingState            _state;
    WorkerControl volatile  _control;
    tatas_lock              _control_lock;
    DataOwnerState volatile _data_owner;
    tatas_lock              _data_owner_lock;

    // data
    ShoreEnv*      _env;    
    partition*     _partition;

    // needed for linked-list of workers
    worker_t<DataType>* _next;
    tatas_lock          _next_lock;

    // statistics
    int            _paused_wait;
    int            _processed;
    tatas_lock     _stats_lock;

    // processor binding
    bool           _is_bound;
    processorid_t  _prs_id;


    // states
    const int _work_PAUSED_impl();
    const int _work_ACTIVE_impl();
    const int _work_STOPPED_impl();

    void _print_stats_impl();

public:

    worker_t(ShoreEnv* env, partition* apart,             
             c_str tname,
             processorid_t aprsid = PBIND_NONE) 
        : thread_t(tname), 
          _env(env), _partition(apart),
          _state(WS_UNDEF), _control(WC_PAUSED), _data_owner(DOS_UNDEF),
          _next(NULL),
          _paused_wait(0), _processed(0),
          _is_bound(false), _prs_id(aprsid)
    {
        assert (_env);
    }

    ~worker_t() { }


    /** access methods */

    // partition related
    void set_partition(partition* apart) {
        assert (apart);
        _partition = apart;
    }

    partition* get_partition() {
        return (_partition);
    }

    // data owner state
    void set_data_owner_state(const DataOwnerState ados) {
        assert ((ados==DOS_ALONE)||(ados==DOS_MULTIPLE));
        CRITICAL_SECTION(dos_cs, _data_owner_lock);
        _data_owner = ados;
    }

    const DataOwnerState get_data_owner_state() {
        return (_data_owner);
    }


    void set_next(worker_t<DataType>* apworker) {
        assert (apworker);
        CRITICAL_SECTION(next_cs, _next_lock);
        _next = apworker;
    }


    // thread control
    bool set_control(WorkerControl awc) {
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
            if ((_control == WC_PAUSED) && 
                ((awc == WC_ACTIVE) || (awc == WC_STOPPED))) {
                _control = awc;
                return (true);
            }

            if ((_control == WC_ACTIVE) && 
                ((awc == WC_PAUSED) || (awc == WC_STOPPED))) {
                _control = awc;
                return (true);
            }
        }
        TRACE( TRACE_DEBUG, "Not allowed transition (%d)-->(%d)\n",
               _control, awc);
        return (false);
    }

    inline WorkerControl get_control() {
        return (_control);
    }

    
    // thread control commands
    inline void stop() {
        set_control(WC_STOPPED);
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
        {
            CRITICAL_SECTION(stats_cs, _stats_lock);
            ++_paused_wait;
        }
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
    while (get_control() == WC_ACTIVE) {
        assert (_partition);
        
        // 1. dequeue an action - it will spin inside the queue
        part_action* pa = NULL;
        do {
            pa = _partition->dequeue(&_control);

            // if signalled to stop
            // propagate signal to next thread in the list, if any
            if (get_control() != WC_ACTIVE) {
                return (0);
            }
        } while (pa == NULL);
        assert (pa);
            
        // 2. attach to xct
        attach_xct(pa->get_xct());
            
        // 3. serve action
        e = pa->trx_exec();
        if (e.is_error()) {
            TRACE( TRACE_ALWAYS, "Problem running xct (%d) [0x%x]\n",
                   pa->get_tid(), e.err_num());
            assert(0);
            return (de_WORKER_RUN_XCT);
        }
            
        // 4. finalize processing
        rvp_t* aprvp = pa->get_rvp();
        assert (aprvp);
        if (aprvp->post()) {
            // last caller
            // execute the code of this rendezvous point
            e = aprvp->run();            
            if (e.is_error()) {
                TRACE( TRACE_ALWAYS, "Problem runing rvp for xct (%d) [0x%x]\n",
                       pa->get_tid(), e.err_num());
                assert(0);
                return (de_WORKER_DETACH_XCT);
            }
            
            // if it was an intermediate RVP it needs to be deattached
            e = aprvp->release(this);
            if (e.is_error()) {
                TRACE( TRACE_ALWAYS, "Problem releasing rvp for xct (%d) [0x%x]\n",
                       pa->get_tid(), e.err_num());
                assert(0);
                return (de_WORKER_DETACH_XCT);
            }
                    
        }
        else {
            // not last caller, simply detach from trx
            TRACE( TRACE_DEBUG, "Deattaching from (%d)\n", pa->get_tid());
            detach_xct(pa->get_xct());
        }

        // 5. update stats
        {
            CRITICAL_SECTION(stats_cs, _stats_lock);
            ++_processed;
        }
    }
    return (0);
}


/****************************************************************** 
 *
 * @fn:     _work_STOPPED_impl()
 *
 * @brief:  Implementation of the STOPPED state. 
 *          (1) It stops, joins and deletes the next thread in the list
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
    CRITICAL_SECTION(stats_cs, _stats_lock);
    TRACE( TRACE_STATISTICS, "Processed (%d)\n", _processed);
    TRACE( TRACE_STATISTICS, "Waited (%d)\n", _paused_wait);
}



EXIT_NAMESPACE(dora);

#endif /** __DORA_WORKER_H */

