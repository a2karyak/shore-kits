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

/** @file:   procstat.h
 *
 *  @brief:  Abstract class (interface) for providing information about the process,
 *           such as CPU, memory, I/O statistics.
 *
 *  @note:   Merged version of the old cpumonitor_t and processorinfo_t functionality
 *
 *  @author: Ippokratis Pandis (ipandis)
 */

/** Typical usage scenario:
    procmonitor_t* monitor = new <subclass>_procmonitor_t();
    monitor->fork(); // if needed
    // <start experiment> //
    monitor->reset();
    // <end experiment> //
    monitor->get_avg_cpu();
 */


#ifndef __UTIL_PROCSTAT_H
#define __UTIL_PROCSTAT_H

#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <vector>
#include <pthread.h>

#include "util/thread.h"
#include "k_defines.h"



/*********************************************************************
 *
 *  @class: cpu_load_values_t
 *
 *  @brief: Used for calculating the measured and offered CPU load 
 *
 *********************************************************************/

struct cpu_load_values_t
{
    double run_tm;  // RunningTime = User time + System time + Other Trap time
    double wait_tm; // Time in runqueue
    cpu_load_values_t() : run_tm(0), wait_tm(0) { }
    ~cpu_load_values_t() { }
};



/*********************************************************************
 *
 *  @class: cpu_measurement
 *
 *  @brief: Used for calculating the CPU utilization (from the timestamp
 *          and the idle nsec, we can calculate the time it spent utilized).
 *
 *********************************************************************/

struct cpu_measurement 
{
    uint64_t timestamp;
    uint64_t cpu_nsec_idle;

    cpu_measurement &operator+=(cpu_measurement const &other);
    cpu_measurement &operator-=(cpu_measurement const &other);
    void clear();
    void set(uint64_t snaptime, uint64_t nsec_idle);
};



/*********************************************************************
 *
 *  @class: procmonitor_t
 *
 *  @brief: Used for monitoring the process statistics. It is implemented
 *          as a thread_t, but it may not be used as a separate thread.
 *
 *********************************************************************/

// Control states for the thread that monitors the cpu usage
enum eCPS { CPS_NOTSET, CPS_RESET, CPS_RUNNING, CPS_PAUSE, CPS_STOP };

// CPU usage monitoring thread 
class procmonitor_t : public thread_t
{ 
protected:

    int _interval_usec;
    double _interval_sec;

    double _total_usage;

    double volatile _num_usage_readings;
    double volatile _avg_usage;

    pthread_mutex_t _mutex;
    pthread_cond_t _cond;

    eCPS volatile _state; 
    virtual void _setup(const double interval_sec);

        
public:

    procmonitor_t(const char* name, 
                 const double interval_sec = 1); // default interval 1 sec
    virtual ~procmonitor_t();

    // Thread entrance
    void work()=0;

    // Control
    void cntr_reset();
    void cntr_pause();
    void cntr_resume();
    void cntr_stop();

    // Statistics -- INTERFACE

    virtual void     stat_reset()=0;

    virtual double   get_avg_usage()=0;
    virtual void     print_avg_usage()=0;
    virtual void     print_ext_stats()=0;
    virtual ulong_t  iochars()=0;
    virtual uint     get_num_of_cpus()=0;

    // for Measured and Offered CPU load
    virtual cpu_load_values_t get_load()=0;
    void print_load(const double delay);

}; // EOF procmonitor_t

#endif /** __UTIL_PROCSTAT_H */
