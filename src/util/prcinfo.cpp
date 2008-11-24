/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   prcinfo.cpp
 * 
 *  @brief:  Implementation of a process information class
 * 
 *  @author: Ippokratis Pandis, Nov 2008
 */

#include "util/prcinfo.h"
#include "util/trace.h"

// constructor opens the file and gets the 
processinfo_t::processinfo_t(const bool printatexit) 
    : _is_ok(0), _print_at_exit(printatexit)
{
    string pathname = "/proc/self/usage";
    if ((_fd = open(pathname.c_str(), O_RDONLY)) < 0) {
        // not good
        TRACE( TRACE_ALWAYS, "Couldn't open /proc/self/usage\n");
    }
    else {

        // goes to the beginning of "file"
        lseek(_fd, 0, SEEK_SET);

        // takes the initial prusage
        if (read(_fd, &_old_prusage, sizeof(_old_prusage)) != sizeof(_old_prusage)) {
            TRACE( TRACE_ALWAYS, "Reading prusage error\n");

        }
        else {
            _timer.reset();
            _is_ok = 1;
        }
    }   
}

processinfo_t::~processinfo_t() { 
    if (_print_at_exit && _is_ok)
        print();
}


const int processinfo_t::reset()
{
    if (!_is_ok) return (1);

    // goes to the beginning of "file"
    lseek(_fd, 0, SEEK_SET);

    // re-reads the prusage
    if (read(_fd, &_prusage, sizeof(_prusage)) != sizeof(_prusage)) {
        TRACE( TRACE_ALWAYS, "Reading prusage error\n");
        return (2);        
    }

    // reset for next call
    _old_prusage = _prusage;
    _timer.reset();


    return (0);
}

const int processinfo_t::print()
{
    if (!_is_ok) return (1);

    // goes to the beginning of "file"
    lseek(_fd, 0, SEEK_SET);

    // re-reads the prusage
    if (read(_fd, &_prusage, sizeof(_prusage)) != sizeof(_prusage)) {
        TRACE( TRACE_ALWAYS, "Reading prusage error\n");
        return (2);        
    }

    // Calculates the difference

    
    // USAGE COUNTERS

    ulong_t pr_minf   = _prusage.pr_minf - _old_prusage.pr_minf;         /* minor page faults */
    ulong_t pr_majf   = _prusage.pr_majf - _old_prusage.pr_majf;         /* major page faults */
    ulong_t pr_nswap  = _prusage.pr_nswap - _old_prusage.pr_nswap;       /* swaps */
    ulong_t pr_inblk  = _prusage.pr_inblk - _old_prusage.pr_inblk;       /* input blocks */
    ulong_t pr_oublk  = _prusage.pr_oublk - _old_prusage.pr_oublk;       /* output blocks */
    ulong_t pr_msnd   = _prusage.pr_msnd - _old_prusage.pr_msnd;         /* messages sent */
    ulong_t pr_mrcv   = _prusage.pr_mrcv - _old_prusage.pr_mrcv;         /* messages received */
    ulong_t pr_sigs   = _prusage.pr_sigs - _old_prusage.pr_sigs;         /* signals received */
    ulong_t pr_vctx   = _prusage.pr_vctx - _old_prusage.pr_vctx;         /* voluntary context switches */
    ulong_t pr_ictx   = _prusage.pr_ictx - _old_prusage.pr_ictx;         /* involuntary context switches */
    ulong_t pr_sysc   = _prusage.pr_sysc - _old_prusage.pr_sysc;         /* system calls */
    ulong_t pr_ioch   = _prusage.pr_ioch - _old_prusage.pr_ioch;         /* chars read and written */


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

    tssub(&pr_rtime, &_prusage.pr_rtime, &_old_prusage.pr_rtime);
    tssub(&pr_utime, &_prusage.pr_utime, &_old_prusage.pr_utime);
    tssub(&pr_stime, &_prusage.pr_stime, &_old_prusage.pr_stime);
    tssub(&pr_ttime, &_prusage.pr_ttime, &_old_prusage.pr_ttime);
    tssub(&pr_tftime, &_prusage.pr_tftime, &_old_prusage.pr_tftime);
    tssub(&pr_dftime, &_prusage.pr_dftime, &_old_prusage.pr_dftime);
    tssub(&pr_kftime, &_prusage.pr_kftime, &_old_prusage.pr_kftime);
    tssub(&pr_ltime, &_prusage.pr_ltime, &_old_prusage.pr_ltime);
    tssub(&pr_slptime, &_prusage.pr_slptime, &_old_prusage.pr_slptime);
    tssub(&pr_wtime, &_prusage.pr_wtime, &_old_prusage.pr_wtime);
    tssub(&pr_stoptime, &_prusage.pr_stoptime, &_old_prusage.pr_stoptime);

    printf("*** Usage Counters *** \n");
    printf("Minor Faults:................. %ld\n", pr_minf);
    printf("Major Faults:................. %ld\n", pr_majf);
    printf("Swaps:........................ %ld\n", pr_nswap);
    printf("Input Blocks:................. %ld\n", pr_inblk);
    printf("Output Blocks:................ %ld\n", pr_oublk);
    printf("STREAMS Messages Sent:........ %ld\n", pr_msnd);
    printf("STREAMS Messages Received:.... %ld\n", pr_mrcv);
    printf("Signals:...................... %ld\n", pr_sigs);
    printf("Voluntary Context Switches:... %ld\n", pr_vctx);
    printf("Involuntary Context Switches:. %ld\n", pr_ictx);
    printf("System Calls:................. %ld\n", pr_sysc);
    printf("Read/Write Characters:........ %ld\n", pr_ioch);
    printf("*** Process Times *** \n");

    long long delay = _timer.time_us();
    prtime("Total Elapsed Time:........... ", delay);
    prtime("Total User Time:.............. ", &pr_utime);
    prtime("Total System Time:............ ", &pr_stime);
    prtime("Other System Trap Time:....... ", &pr_ttime);
    prtime("Text Page Fault Sleep Time.... ", &pr_tftime);
    prtime("Data Page Fault Sleep Time.... ", &pr_dftime);
    prtime("Kernel Page Fault Sleep Time.. ", &pr_kftime);
    prtime("User Lock Wait Sleep Time..... ", &pr_ltime);
    prtime("All Other Sleep Time.......... ", &pr_slptime);
    prtime("Time Waiting for a CPU........ ", &pr_wtime);
    prtime("Stopped Time.................. ", &pr_stoptime);

    // reset for next call
    _old_prusage = _prusage;
    _timer.reset();

    return (0);   
}



void processinfo_t::tsadd(timestruc_t* result, timestruc_t *a, timestruc_t *b)
{
    result->tv_sec = a->tv_sec + b->tv_sec;
    if ((result->tv_nsec = a->tv_nsec + b->tv_nsec) >= 1000000000) {
        result->tv_nsec -= 1000000000;
        result->tv_sec += 1;
    }
}


void processinfo_t::tssub(timestruc_t* result, timestruc_t *a, timestruc_t *b)
{
    result->tv_sec = a->tv_sec - b->tv_sec;
    if ((result->tv_nsec = a->tv_nsec - b->tv_nsec) < 0) {
        result->tv_nsec += 1000000000;
        result->tv_sec -= 1;
    }
}


void processinfo_t::hr_min_sec(char* buf, long sec)
{
    sprintf(buf, "%ld", sec);
    return;

    if (sec >= 3600)
        sprintf(buf, "%ld:%.2ld:%.2ld",
                sec / 3600, (sec % 3600) / 60, sec % 60);
    else if (sec >= 60)
        sprintf(buf, "%ld:%.2ld",
                sec / 60, sec % 60);
    else {
        sprintf(buf, "%ld", sec);
    }
}


void processinfo_t::prtime(string label, timestruc_t* ts)
{
    char buf[32];
    hr_min_sec(buf, ts->tv_sec);
    cerr << label << buf << "." << (u_int)ts->tv_nsec/1000000 << endl;
}


void processinfo_t::prtime(string label, long long& delay)
{
    char buf[32];
    hr_min_sec(buf, delay*1e-6);
    cerr << label << buf << "." << delay/100000 << endl;
}



