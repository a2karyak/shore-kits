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
 * @class worker_t
 *
 * @brief An smthread-based class for the worker threads
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
    WorkerControl  _control;
    DataOwnerState _data_owner;
    tatas_lock     _state_lock;

    // data
    ShoreEnv*      _env;    
    partition*     _partition;

    // statistics
    int            _paused_wait;
    int            _processed;

public:

    worker_t(ShoreEnv* env, partition* apart,
             c_str tname) 
        : thread_t(tname), 
          _env(env), _partition(apart),
          _state(WS_UNDEF), _control(WC_PAUSED), _data_owner(DOS_UNDEF),
          _paused_wait(0), _processed(0)
    {
        assert (_env);
    }

    ~worker_t() { 
        TRACE( TRACE_DEBUG, 
    }


    /** access methods */
    
    void set_control(WorkerControl awc) {
        // only two transitions are allowed
        // paused -> active -> stopped
        CRITICAL_SECTION(wtcs, _state_lock);
        if ((_control == WC_PAUSED) && (awc == WC_ACTIVE))
            _control = WC_ACTIVE;
        elseif ((_control == WC_ACTIVE) && (awc == WC_STOPPED))
            _control = WC_STOPPED;
    }

    WorkerControl get_control() {
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

        // while paused loop and sleep
        while (_control == WC_PAUSED)
            paused++;

        // do active loop
        while (_control == WC_ACTIVE) {
            assert (_partition);
        
            // 1. dequeue an action
            part_action* pa = _partition.dequeue();
            
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

