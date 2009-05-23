/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
      Data Intensive Applications and Systems Labaratory (DIAS)
               Ecole Polytechnique Federale de Lausanne
   
                         All Rights Reserved.
   
   Permission to use, copy, modify and distribute this software and
   its documentation is hereby granted, provided that both the
   copyright notice and this permission notice appear in all copies of
   the software, derivative works or modified versions, and any
   portions thereof, and that both notices appear in supporting
   documentation.
   
   This code is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
   DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
   RESULTING FROM THE USE OF THIS SOFTWARE.
*/

/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_worker.h
 *
 *  @brief:  Wrapper for Shore worker threads
 *
 *  @author: Ippokratis Pandis, Oct 2008
 */


#ifndef __SHORE_WORKER_H
#define __SHORE_WORKER_H

#include <cstdio>

// for binding LWP to cores
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>


#include "util.h"
#include "shore.h"


ENTER_NAMESPACE(shore);


// Use this to enable verbode stats for worker threads
#undef WORKER_VERBOSE_STATS
#define WORKER_VERBOSE_STATS

// ditto
#undef WORKER_VERY_VERBOSE_STATS
//#define WORKER_VERY_VERBOSE_STATS


const int WAITING_WINDOW = 5; // keep track the last 5 seconds



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

    int _served_waiting;

    int _checked_input;
    int _served_input;
    
    int _condex_sleep;
    int _failed_sleep;

#ifdef WORKER_VERBOSE_STATS
    void update_served(const double serve_time_ms);
    double _serving_total;   // in msecs

    void update_rvp_exec_time(const double rvp_exec_time);
    void update_rvp_notify_time(const double rvp_notify_time);
    double _rvp_exec;
    double _rvp_notify;

    void update_waited(const double queue_time);
    double _waiting_total; // not only the last WAITING_WINDOW secs
#ifdef WORKER_VERY_VERBOSE_STATS
    double _ww[WAITING_WINDOW];
    int _ww_idx; // index on the ww (waiting window) ring
    stopwatch_t _for_last_change;
    double _last_change;
#endif 
#endif

    worker_stats_t() 
        : _processed(0), _problems(0),
          _served_waiting(0),
          _checked_input(0), _served_input(0),
          _condex_sleep(0), _failed_sleep(0)
#ifdef WORKER_VERBOSE_STATS
        , _waiting_total(0), _serving_total(0), 
          _rvp_exec(0), _rvp_notify(0)
#ifdef WORKER_VERY_VERBOSE_STATS
        , _ww_idx(0), _last_change(0)
#endif
#endif
    { }

    ~worker_stats_t() { }
      
    void print_stats() const;

    void reset();

    void print_and_reset() { print_stats(); reset(); }

    worker_stats_t& operator+=(worker_stats_t const& rhs);

}; // EOF: worker_stats_t



/******************************************************************** 
 *
 * @class: base_worker_t
 *
 * @brief: An smthread-based non-template abstract base class for the 
 *         Shore worker threads
 *
 * @note:  By default the worker thread is not bound to any processor.
 *         The creator of the worker thread needs to
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
    worker_stats_t _stats;

    // processor binding
    bool                     _is_bound;
    processorid_t            _prs_id;

    // states
    virtual const int _work_PAUSED_impl();
    virtual const int _work_ACTIVE_impl()=0;
    virtual const int _work_STOPPED_impl();
    virtual const int _pre_STOP_impl()=0;


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

    virtual ~base_worker_t() { }    

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
    inline eWorkingState set_ws(const eWorkingState new_ws) {
        CRITICAL_SECTION(ws_cs, _ws_lock);
        eWorkingState old_ws = *&_ws;

        if ((old_ws==WS_COMMIT_Q)&&(new_ws!=WS_LOOP)) {
            // ignore notice if flag is already set to commit
            return (old_ws); 
        }
        _ws=new_ws;
        ws_cs.exit();
        if ((old_ws==WS_SLEEP)&&(new_ws!=WS_SLEEP)) {
            // wake up if sleeping
            //membar_producer();
            condex_wakeup();

        }
        return (old_ws);
    }

    inline eWorkingState get_ws() { return (*&_ws); }


    inline const bool can_continue(const eWorkingState my_ws) {
        //    CRITICAL_SECTION(ws_cs, _ws_lock);
        return ((*&_ws==my_ws)||(*&_ws==WS_LOOP));
    }


    inline const bool can_continue_cs(const eWorkingState my_ws) {
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

        int rval = 0;

        // state machine
        while (true) {
            switch (get_control()) {
            case (WC_PAUSED):
                if (work_PAUSED())
                    return;
                break;

            case (WC_ACTIVE):

                _env->env_thread_init();

                // does the real work
                rval = work_ACTIVE();

                _env->env_thread_fini();

                if (rval)
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

    const bool abort_one_trx(xct_t* axct);

    void stats(); 

    worker_stats_t get_stats() { return (*&_stats); }
    void reset_stats() { _stats.reset(); }

private:

    // copying not allowed
    base_worker_t(base_worker_t const &);
    void operator=(base_worker_t const &);
    
}; // EOF: base_worker_t


EXIT_NAMESPACE(shore);

#endif /** __SHORE_WORKER_H */

