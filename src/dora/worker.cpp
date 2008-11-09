/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file worker.cpp
 *
 *  @brief Declaration of the worker threads in DORA
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#include "dora/worker.h"


ENTER_NAMESPACE(dora);


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
    TRACE( TRACE_DEBUG, "Pausing...\n");

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

void base_worker_t::_print_stats_impl() const
{
    TRACE( TRACE_TRX_FLOW, "Processed (%d)\n", _processed);
    TRACE( TRACE_TRX_FLOW, "Problems  (%d)\n", _problems);
}


EXIT_NAMESPACE(dora);
