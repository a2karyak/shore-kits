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

/** @file:   cpustat.h
 *
 *  @brief:  Class that provides information about cpu usage during runs
 *
 *  @note:   Needs to compile with -lkstat -lrt
 *
 *  @author: Ippokratis Pandis (ipandis)
 *  @author: Ryan Johnson (ryanjohn)
 */

/** Typical usage scenario:
    cpumonitor_t* cputhread = new cpumonitor_t();
    cputhread->fork();
    // <start experiment> //
    cputhread->reset();
    // <end experiment> //
    cputhread->pause();
    cpu_usage_info cui = cputhread->get_usage();
    cui.info();
 */

#ifndef __UTIL_CPUSTAT_H
#define __UTIL_CPUSTAT_H

#include <stdio.h>
#include <string.h>
#include <kstat.h>
#include <unistd.h>
#include <vector>
#include <pthread.h>

#include "util.h"

#ifdef __sparcv9
#include <atomic.h>
#endif

struct cpu_measurement 
{
    uint64_t timestamp;
    uint64_t cpu_nsec_idle;

    cpu_measurement &operator+=(cpu_measurement const &other) {
	timestamp += other.timestamp;
	cpu_nsec_idle += other.cpu_nsec_idle;
	return (*this);
    }
    cpu_measurement &operator-=(cpu_measurement const &other) {
	timestamp -= other.timestamp;
	cpu_nsec_idle -= other.cpu_nsec_idle;
	return (*this);
    }
    void clear() {
        timestamp = 0;
        cpu_nsec_idle = 0;
    }
    void set(uint64_t snaptime, uint64_t nsec_idle) {
        timestamp = snaptime;
        cpu_nsec_idle = nsec_idle;
    }
};

struct kstat_entry 
{
    kstat_t*	ksp;
    long	offset;
    cpu_measurement measured[2];
};


// Control states for the thread that monitors the cpu usage
enum eCPS { CPS_NOTSET, CPS_RESET, CPS_RUNNING, CPS_PAUSE, CPS_STOP };

// CPU usage monitoring thread 
class cpumonitor_t : public thread_t
{ 
private: 

    std::vector<kstat_entry> _entries;
    int _interval_usec;
    double _interval_sec;
    kstat_ctl_t* _pkc;

    void _setup(const double interval_sec);

    double _total_usage;

    double volatile _num_usage_readings;
    double volatile _avg_usage;
    eCPS volatile _state; 

    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
        

public:

    cpumonitor_t(const double interval_sec = 1) // default interval 1 sec
        : thread_t("cpumon"), 
          _interval_usec(0), _interval_sec(interval_sec), _state(CPS_NOTSET),
          _total_usage(0), _num_usage_readings(0), _avg_usage(0),
          _pkc(NULL)
    { 
        _setup(interval_sec);
    }

    ~cpumonitor_t() { 
        if (*&_setup != CPS_NOTSET) {            
            pthread_mutex_destroy(&_mutex);
            pthread_cond_destroy(&_cond);
            kstat_close(_pkc);
        }
    }

    // Thread entrance
    void work();

    // Control
    void reset()  { _state = CPS_RESET; }
    void pause()  { _state = CPS_PAUSE; }
    void resume() { _state = CPS_RUNNING; }
    void stop()   { _state = CPS_STOP; }

    // Access methods
    double get_avg_usage() { return (*&_avg_usage); }
    void print_avg_usage() { 
        double au = *&_avg_usage;
        double entriessz = _entries.size();
        TRACE( TRACE_STATISTICS, 
               "\nAvgCPU:       (%.1f) (%.1f%%)\n",
               au, 100*(au/entriessz));
    }

}; // EOF cpumonitor_t

#endif /** __UTIL_CPUSTAT_H */
