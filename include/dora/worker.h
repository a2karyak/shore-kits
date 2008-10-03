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
#include "sm/shore/shore_env.h"


ENTER_NAMESPACE(dora);


using namespace shore;



/******************************************************************** 
 *
 * @enum  WorkerState
 *
 * @brief Possible states of a worker thread
 *
 * @note  Finished = Done assigned job but still trx not decided
 *        (Finished != Idle)
 *
 ********************************************************************/

enum WorkerState { WS_UNDEF, WS_IDLE, WS_ASSIGNED, WS_FINISHED };



/******************************************************************** 
 *
 * @enum  WorkerControl
 *
 * @brief States for controling a worker thread
 *
 * @note  A thread initially is paused then it starts working until
 *        someone changes the status to stopped.
 *
 ********************************************************************/

enum WorkerControl { WC_PAUSED, WC_ACTIVE, WC_STOPPED };


/******************************************************************** 
 *
 * @enum  DataOwnerState
 *
 * @brief Owning status for the worker thread of the data partition
 *
 ********************************************************************/

enum DataOwnerState { DOS_UNDEF, DOS_ALONE, DOS_MULTIPLE };



/******************************************************************** 
 *
 * @class: worker_t
 *
 * @brief: An smthread-based class for the worker threads
 *
 * @note:  By default the worker thread is not bound to any processor.
 *         The creator of the worker thread (the partition) needs to
 *         decide where and if it will bind it somewhere.
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
    WorkerState    _state;
    volatile WorkerControl _control;
    tatas_lock     _control_lock;
    DataOwnerState _data_owner;

    // data
    ShoreEnv*      _env;    
    partition*     _partition;

    // needed for linked-list of workers
    worker_t<DataType>* _next;
    tatas_lock          _next_lock;

    // statistics
    int            _paused_wait;
    int            _processed;

    // processor binding
    bool           _is_bound;
    processorid_t  _prs_id;

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

    ~worker_t() { 
        TRACE( TRACE_STATISTICS, "Processed (%d) - waited (%d)\n",
               _processed, _paused_wait);
    }


    /** access methods */
    
    bool set_control(WorkerControl awc) {
        // only two transitions are allowed
        // paused -> active -> stopped
        CRITICAL_SECTION(wtcs, _control_lock);
        if ((_control == WC_PAUSED) && (awc == WC_ACTIVE)) {
            _control = WC_ACTIVE;
            return (true);
        }

        if ((_control == WC_ACTIVE) && (awc == WC_STOPPED)) {
            _control = WC_STOPPED;
            return (true);
        }

        return (false);
    }

    inline WorkerControl get_control() {
        CRITICAL_SECTION(wtcs, _control_lock);
        return (_control);
    }

    
    void set_partition(partition* apart) {
        assert (apart);
        _partition = apart;
    }

    partition* get_partition() {
        return (_partition);
    }


    // thread entrance
    inline void work() {

        // state (WC_PAUSED)

        // while paused loop and sleep
        while (get_control() == WC_PAUSED) {
            paused++;
            sleep(1);
        }


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

        // do active loop
        while (get_control() == WC_ACTIVE) {
            assert (_partition);
        
            // 1. dequeue an action
            part_action* pa = NULL;
            do {
                pa = _partition.dequeue();
                // do loop as long as status is active
                if (get_control() != WC_ACTIVE)
                    return;
            } while (pa == NULL);
            assert (pa);
            
            // 2. attach to xct
            ss_m::attach_xct(pa->get_xct());
            
            // 3. serve action
            pa->trx_exec();
            
            // 4. commit - use default commit
            if (pa->get_rvp()->post()) {
                // last caller
                pa->trx_rvp();
            }
            else {
                // detach
                ss_m::detach_xct(pa->get_xct());
            }
        }
    }


}; // EOF: worker_t



EXIT_NAMESPACE(dora);

#endif /** __DORA_WORKER_H */

