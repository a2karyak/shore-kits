/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file    : tpch_q1.h
 *  @brief   : TPCH Query 1
 *  @version : 0.1
 *  @history :
 7/8/2006: Initial version
*/


#include "workload/job.h"
#include "workload/common.h"
#include "workload/dispatcher_globals.h"
#include "engine/dispatcher.h"

using namespace qpipe;


class tpch_q6 : public job_t {
private:
    int randtest;

public:

    tpch_q6() : job_t() {

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

        packet_t* q6 =
            create_q6_packet( "Q6_CLIENT_", global_dispatcher_policy_get() );
        tuple_buffer_t* out = q6->_output_buffer;
        
        dispatcher_t::dispatch_packet(q6);
        tuple_t output;
        while( !out->get_tuple(output) ) {
            double* r = (double*)output.data;
            TRACE(TRACE_QUERY_RESULTS, "*** Q6 Count: %u. Sum: %lf.  ***\n", (unsigned)r[0], r[1]);
        }

        return NULL;

    }

}; // END OF CLASS: tpch_q6



class tpch_q6_driver : public job_driver_t {
 public:
  
    tpch_q6_driver() { }
    ~tpch_q6_driver() { }

    
    void* drive( ) {
        
        TRACE( TRACE_DEBUG, "Driving a TPC-H Q6\n");

        tpch_q6* a_tpch_q6 = new tpch_q6();
        
        return (a_tpch_q6->start());
    }

    void print_info ( ) {

        TRACE( TRACE_DEBUG, "TPCH Q1 DRIVER\n");
    }
    
}; // END OF CLASS: tpch_q1_driver
