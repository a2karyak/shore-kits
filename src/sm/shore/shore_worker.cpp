/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_worker.cpp
 *
 *  @brief:  Declaration of the worker threads in Shore
 *
 *  @author: Ippokratis Pandis (ipandis)
 */


#include "sm/shore/shore_worker.h"


ENTER_NAMESPACE(shore);



/****************************************************************** 
 *
 * @fn:     print_stats(), reset()
 *
 * @brief:  Helper functions for the worker stats
 * 
 ******************************************************************/

void worker_stats_t::print_stats() const
{
    //    TRACE( TRACE_STATISTICS, "Processed      (%d)\n", _processed);
    //    TRACE( TRACE_STATISTICS, "Problems       (%d)\n", _problems);

    // don't display anything if no input
    if (_served_input==0) {
        TRACE( TRACE_STATISTICS, "No input\n");
        return;
    }

    TRACE( TRACE_STATISTICS, "Input checked  (%d)\n", _checked_input);
    TRACE( TRACE_STATISTICS, "Input served   (%d)\n", _served_input);

    TRACE( TRACE_STATISTICS, "Wait checked   (%d)\n", _checked_waiting);
    TRACE( TRACE_STATISTICS, "Wait served    (%d)\n", _served_waiting);

    TRACE( TRACE_STATISTICS, "Sleeped        (%d)\n", _condex_sleep);
    //    TRACE( TRACE_STATISTICS, "Failed sleeped (%d)\n", _failed_sleep);
    
    TRACE( TRACE_STATISTICS, "Early Aborts   (%d)\n", _early_aborts);
    TRACE( TRACE_STATISTICS, "Midway Aborts  (%d)\n", _mid_aborts);
}

void worker_stats_t::reset()
{
    _processed = 0;
    _problems  = 0;

    _checked_waiting = 0;
    _served_waiting  = 0;

    _checked_input = 0;
    _served_input  = 0;

    _condex_sleep = 0;
    _failed_sleep = 0;
}



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



EXIT_NAMESPACE(shore);
