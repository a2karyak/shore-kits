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
    worker_stats_t           _stats;

    // processor binding
    bool                     _is_bound;
    processorid_t            _prs_id;

    // sli
    int                      _use_sli;

    // states
    virtual const int _work_PAUSED_impl();
    virtual const int _work_ACTIVE_impl()=0;
    virtual const int _work_STOPPED_impl();

    void _print_stats_impl() const;

public:

    base_worker_t(ShoreEnv* env, c_str tname, processorid_t aprsid, const int use_sli) 
        : thread_t(tname), 
          _env(env),
          _control(WC_PAUSED), _data_owner(DOS_UNDEF), _ws(WS_UNDEF),
          _next(NULL), _is_bound(false), _prs_id(aprsid), _use_sli(use_sli)
    {
        assert (_env);        
    }

    virtual ~base_worker_t() { 
        //        stats();
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

        // 3. set SLI option
        ss_m::set_sli_enabled(_use_sli);


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

    void stats() { 
        TRACE( TRACE_STATISTICS, "(%s)\n", 
               thread_name().data());
        _stats.print_stats(); 
    }

private:

    // copying not allowed
    base_worker_t(base_worker_t const &);
    void operator=(base_worker_t const &);

    
}; // EOF: base_worker_t



EXIT_NAMESPACE(shore);

#endif /** __SHORE_WORKER_H */

