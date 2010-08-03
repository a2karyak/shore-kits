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

/** @file:   procstat.cpp
 *
 *  @brief:  Abstract class that provides information about
 *           the CPU usage during runs
 *
 *  @author: Ippokratis Pandis (ipandis)
 */

#include "util/procstat.h"

#include "util.h"


cpu_measurement& cpu_measurement::operator+=(cpu_measurement const &other) 
{
    timestamp += other.timestamp;
    cpu_nsec_idle += other.cpu_nsec_idle;
    return (*this);
}

cpu_measurement& cpu_measurement::operator-=(cpu_measurement const &other) 
{
    timestamp -= other.timestamp;
    cpu_nsec_idle -= other.cpu_nsec_idle;
    return (*this);
}

void cpu_measurement::clear() {
    timestamp = 0;
    cpu_nsec_idle = 0;
}

void cpu_measurement::set(uint64_t snaptime, uint64_t nsec_idle) 
{
    timestamp = snaptime;
    cpu_nsec_idle = nsec_idle;
}



/*********************************************************************
 *
 *  @class: procmonitor_t
 *
 *********************************************************************/

procmonitor_t::procmonitor_t(const char* name, const double interval_sec)
    : thread_t(name), _interval_usec(0), _interval_sec(interval_sec),
      _total_usage(0), _num_usage_readings(0), _avg_usage(0),
      _state(CPS_NOTSET)
{
}

procmonitor_t::~procmonitor_t()
{
    if (*&_state != CPS_NOTSET) {            
        pthread_mutex_destroy(&_mutex);
        pthread_cond_destroy(&_cond);
    }
}


void procmonitor_t::_setup(const double interval_sec) 
{
    // set interval
    assert (interval_sec>0);
    _interval_usec = int(interval_sec*1e6);

    if (interval_sec<1)
        TRACE( TRACE_DEBUG, "CPU usage updated every (%0.3f) msec\n", 
               _interval_usec/1000.);
    else
	TRACE( TRACE_DEBUG, "CPU usage(updated every (%0.6f) sec\n", 
               _interval_usec/1000./1000);

    // setup cond
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);  

    _state = CPS_PAUSE;
}


void procmonitor_t::print_load(const double delay)
{
    const cpu_load_values_t load = get_load();

    // Print only if got meaningful numbers
    if ((load.run_tm>0.) && (load.wait_tm>0.)) {
        TRACE( TRACE_ALWAYS, "\nCpuLoad = (%.2f)\nAbsLoad = (%.2f)\n", 
               (load.run_tm+load.wait_tm)/load.run_tm, load.run_tm/delay);
    }
}



// Control
void procmonitor_t::cntr_reset()  
{ 
    stat_reset();
    _state = CPS_RESET; 
}

void procmonitor_t::cntr_pause()  
{ 
    _state = CPS_PAUSE; 
}

void procmonitor_t::cntr_resume() 
{ 
    _state = CPS_RUNNING; 
}

void procmonitor_t::cntr_stop()   
{ 
    _state = CPS_STOP; 
}
