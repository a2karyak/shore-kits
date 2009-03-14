/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_worker.cpp
 *
 *  @brief:  Declaration of the worker threads in Shore
 *
 *  @author: Ippokratis Pandis (ipandis)
 */


#include "sm/shore/shore_worker.h"


ENTER_NAMESPACE(shore);


#ifdef WORKER_VERBOSE_STATS
#warning Verbose worker statistics enabled
#endif



/****************************************************************** 
 *
 * @fn:     print_stats(), reset()
 *
 * @brief:  Helper functions for the worker stats
 * 
 ******************************************************************/

void worker_stats_t::print_stats() const
{
    TRACE( TRACE_STATISTICS, "Processed      (%d)\n", _processed);
    TRACE( TRACE_STATISTICS, "Failure rate   (%d) \t%.1f%%\n", 
           _problems, (double)(100*_problems)/(double)_processed);

    TRACE( TRACE_STATISTICS, "Input checked  (%d)\n", _checked_input);

    TRACE( TRACE_STATISTICS, "Input served   (%d) \t%.1f%%\n", 
           _served_input, (double)(100*_served_input)/(double)_checked_input);

    TRACE( TRACE_STATISTICS, "Wait served    (%d) \t%.1f%%\n", 
           _served_waiting, (double)(100*_served_waiting)/(double)_checked_input);
    
    TRACE( TRACE_STATISTICS, "Early aborts   (%d) \t%.1f%%\n", 
           _early_aborts, (double)(100*_early_aborts)/(double)_checked_input);

#ifdef MIDWAY_ABORTS
    TRACE( TRACE_STATISTICS, "Midway aborts  (%d) \t%.1f%%\n", 
           _mid_aborts, (double)(100*_mid_aborts)/(double)_checked_input);
#endif

    TRACE( TRACE_STATISTICS, "Sleeped        (%d) \t%.1f%%\n", 
           _condex_sleep, (double)(100*_condex_sleep)/(double)_checked_input);
    TRACE( TRACE_STATISTICS, "Failed sleeped (%d) \t%.1f%%\n", 
           _failed_sleep, (double)(100*_failed_sleep)/(double)_checked_input);


#ifdef WORKER_VERBOSE_STATS

    TRACE( TRACE_STATISTICS, "Avg  serving   (%.3fms)\n", 
           _serving_total/(double)(_processed-_early_aborts));
    
    TRACE( TRACE_STATISTICS, "Total wait     (%.2f)\n", _waiting_total);

    for (int i=1; i<WAITING_WINDOW; i++) {
        TRACE( TRACE_STATISTICS, "(%d -> %d). Wait (%.2f)\n",
               i, i+1, _ww[(_ww_idx+i)%WAITING_WINDOW]);        
    }

#endif

}

void worker_stats_t::reset()
{
    _processed = 0;
    _problems  = 0;

    _served_waiting  = 0;

    _checked_input = 0;
    _served_input  = 0;

    _condex_sleep = 0;
    _failed_sleep = 0;

    _early_aborts = 0;

#ifdef MIDWAY_ABORTS    
    _mid_aborts = 0;
#endif

#ifdef WORKER_VERBOSE_STATS
    _waiting_total = 0;
    for (int i=0; i<WAITING_WINDOW; i++) _ww[i] = 0;
    _ww_idx = 0;
    _last_change = 0;
    _serving_total = 0;
#endif
}


#ifdef WORKER_VERBOSE_STATS

void worker_stats_t::update_waited(const double queue_time)
{
    _waiting_total += queue_time;
    _last_change += _for_last_change.time(); 
    if (_last_change>1) {
        // if more than 1 sec passed since the last ww_idx change, move it
        _ww_idx = (_ww_idx + 1) % WAITING_WINDOW;
        _ww[_ww_idx] = 0;
        _last_change = 0;
    }
    // update waiting window (ww)
    _ww[_ww_idx] += queue_time;
}

void worker_stats_t::update_served(const double serve_time_ms)
{
    _serving_total += serve_time_ms;
}

#endif // WORKER_VERBOSE_STATS


/****************************************************************** 
 *
 * @fn:     _work_PAUSED_impl()
 *
 * @brief:  Implementation of the PAUSED state
 *
 * @return: 0 on success
 * 
 ******************************************************************/

const int base_worker_t::_work_PAUSED_impl()
{
    //    TRACE( TRACE_DEBUG, "Pausing...\n");

    // state (WC_PAUSED)

    // while paused loop and sleep
    while (get_control() == WC_PAUSED) {
        sleep(1);
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

const int base_worker_t::_work_STOPPED_impl()
{
    //    TRACE( TRACE_DEBUG, "Stopping...\n");

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
    //    stats();

    return (0);
}




#ifdef ACCESS_RECORD_TRACE
#warning Tracing record accesses enabled
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
void base_worker_t::create_trace_dir(string dir)
{
    mkdir(dir.c_str(),0777);
}
void base_worker_t::open_trace_file()
{
    string dir = envVar::instance()->getVar("dir-trace","RAT");
    create_trace_dir(dir);
    time_t second = time(NULL);
    stringstream st;
    st << second;
    string fname = dir + string("/rat-") + st.str() + string("-") + this->thread_name() + string(".csv");
    TRACE( TRACE_ALWAYS, "Opening file (%s)\n", fname.c_str());
    _trace_file.open(fname.c_str());
}
void base_worker_t::close_trace_file()
{
    for (vector<string>::iterator it = _events.begin(); it != _events.end(); it++) {
        _trace_file << this->thread_name() << "," << *it << endl;
    }
    _trace_file.close();
    TRACE( TRACE_ALWAYS, "Closing file. (%d) events dumped\n", _events.size());
}
#endif // ACCESS_RECORD_TRACE



EXIT_NAMESPACE(shore);
