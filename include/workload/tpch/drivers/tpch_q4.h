/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file    : tpch_q4.h
 *  @brief   : TPCH Query 6
 *  @version : 0.1
 *  @history :
 7/8/2006: Initial version
*/


#include "workload/job.h"
#include "workload/common.h"
#include "workload/dispatcher_globals.h"
#include "engine/dispatcher.h"


using namespace qpipe;


class tpch_q4 : public job_t {
private:
    int randtest;

public:

    tpch_q4() : job_t() {

        job_cmd = "1";
        job_desc = "TPCH-1";
        
        init();
        print_predicates();
    }
    
    /* Initialize */
    virtual void init() {
        TRACE( TRACE_DEBUG, "TPCH Q1 Initializing. id=%s\n", job_cmd.c_str());

        gettimeofday(&tv, 0);
        mv = tv.tv_usec * getpid();

        randtest = rand_r(&mv);
    }
        
    /* Start executing */
    virtual void* start() {
        TPCH_Q4::run( global_dispatcher_policy_get() );
        return NULL;
    }

}; // END OF CLASS: tpch_q4



class tpch_q4_driver : public job_driver_t {
 public:
  
    tpch_q4_driver() { }
    ~tpch_q4_driver() { }

    
    void* drive( ) {
        
        TRACE( TRACE_DEBUG, "Driving a TPC-H Q4\n");

        tpch_q4* a_tpch_q4 = new tpch_q4();
        
        return (a_tpch_q4->start());
    }

    void print_info ( ) {

        TRACE( TRACE_DEBUG, "TPCH Q1 DRIVER\n");
    }
    
}; // END OF CLASS: tpch_q1_driver
