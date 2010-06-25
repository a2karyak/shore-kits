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

/** @file:   prcinfo.h
 * 
 *  @brief:  Processor usage information
 * 
 *  @author: Ippokratis Pandis, Nov 2008
 */


/*
 * Prints all field resource, usage and microstat accounting fields
 *
 * @note: Taken from - http://my.safaribooksonline.com/0131482092/app02lev1sec2
 * @note: Look also  - http://docs.sun.com/app/docs/doc/816-5174/proc-4
 *
 */


#ifndef __UTIL_PRCINFO_H
#define __UTIL_PRCINFO_H


#include <sys/types.h>
#include <sys/time.h>

#ifdef __sparcv9
#include <procfs.h>
#else
#include <sys/procfs.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

#include "k_defines.h"

#include "util/stopwatch.h"

#include <iostream>

using namespace std;


struct load_values_t
{
    double run_tm;  // RunningTime = User time + System time + Other Trap time
    double wait_tm; // Time in runqueue
    load_values_t() : run_tm(0), wait_tm(0) { }
    ~load_values_t() { }
};


#ifndef __sparcv9


typedef struct timespec timestruc_t;

struct prusage 
{
    id_t pr_lwpid;           /* lwp id.  0: process or defunct */
    int pr_count;            /* number of contributing lwps */
    timestruc_t pr_tstamp;   /* real time stamp, time of read() */
    timestruc_t pr_create;   /* process/lwp creation time stamp */
    timestruc_t pr_term;     /* process/lwp termination time stamp */
    timestruc_t pr_rtime;    /* total lwp real (elapsed) time */
    timestruc_t pr_utime;    /* user level CPU time */
    timestruc_t pr_stime;    /* system call CPU time */
    timestruc_t pr_ttime;    /* other system trap CPU time */
    timestruc_t pr_tftime;   /* text page fault sleep time */
    timestruc_t pr_dftime;   /* data page fault sleep time */
    timestruc_t pr_kftime;   /* kernel page fault sleep time */
    timestruc_t pr_ltime;    /* user lock wait sleep time */
    timestruc_t pr_slptime;  /* all other sleep time */
    timestruc_t pr_wtime;    /* wait-cpu (latency) time */
    timestruc_t pr_stoptime; /* stopped time */
    ulong_t pr_minf;         /* minor page faults */
    ulong_t pr_majf;         /* major page faults */
    ulong_t pr_nswap;        /* swaps */
    ulong_t pr_inblk;        /* input blocks */
    ulong_t pr_oublk;        /* output blocks */
    ulong_t pr_msnd;         /* messages sent */
    ulong_t pr_mrcv;         /* messages received */
    ulong_t pr_sigs;         /* signals received */
    ulong_t pr_vctx;         /* voluntary context switches */
    ulong_t pr_ictx;         /* involuntary context switches */
    ulong_t pr_sysc;         /* system calls */
    ulong_t pr_ioch;         /* chars read and written */
};

typedef prusage prusage_t;

#endif

struct processinfo_t 
{
    int _fd;

    prusage_t   _old_prusage;
    prusage_t   _prusage;
    stopwatch_t _timer;

    bool _print_at_exit;
    int _is_ok;

    processinfo_t(const bool printatexit = false);
    ~processinfo_t();

    // prints information and resets
    int reset();
    int print();
    ulong_t iochars();
    load_values_t getload();

    static double trans(timestruc_t ats);

    static void hr_min_sec(char*, long);
    static void prtime(string label, timestruc_t* ts);
    static void prtime(string label, long long& delay);

    static void tsadd(timestruc_t* result, timestruc_t* a, timestruc_t* b);
    static void tssub(timestruc_t* result, timestruc_t* a, timestruc_t* b);

}; // EOF: processinfo_t

#endif /** __UTIL_PRCINFO_H */
