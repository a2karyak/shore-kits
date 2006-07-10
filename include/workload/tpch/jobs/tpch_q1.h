/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file    : tpch_q1.h
 *  @brief   : TPCH Query 1
 *  @version : 0.1
 *  @history :
 7/8/2006: Initial version
*/


#include "workload/job.h"
#include "engine/core/tuple.h"
#include "engine/stages/tscan.h"
#include "engine/stages/partial_aggregate.h"
#include "engine/dispatcher/dispatcher_policy.h"

#include "workload/common.h"
#include "workload/dispatcher_globals.h"
#include "engine/dispatcher.h"


using namespace qpipe;


class tpch_q1 : public job_t {
private:
    int randtest;

public:

    tpch_q1() : job_t() {

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
    
    /* Print predicates */
    virtual void print_predicates() {
        TRACE( TRACE_DEBUG, "Printing Job Predicates\n");

        TRACE( TRACE_DEBUG, "%d\n", randtest);
    }
    
    /* Start executing */
    virtual void* start() {
        TPCH_Q1::run( global_dispatcher_policy_get() );
        return NULL;
    }
    
}; // END OF CLASS: tpch_q1



class tpch_q1_driver : public job_driver_t {
 public:
  
    tpch_q1_driver() { }
    ~tpch_q1_driver() { }

    
    void* drive( ) {
        
        TRACE( TRACE_DEBUG, "Driving a TPC-H Q1\n");

        tpch_q1* a_tpch_q1 = new tpch_q1();
        
        return (a_tpch_q1->start());
    }

    void print_info ( ) {

        TRACE( TRACE_DEBUG, "TPCH Q1 DRIVER\n");
    }
    
}; // END OF CLASS: tpch_q1_driver



