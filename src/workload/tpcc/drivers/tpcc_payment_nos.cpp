/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file test_payment_single_thr.cpp
 *
 *  @brief Test running TPC-C PAYMENT_BASELINE transaction without staging
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "workload/workload.h"
#include "workload/tpcc/drivers/tpcc_payment_nos.h"
#include "workload/tpcc/drivers/tpcc_payment_single_thr.h"

#include "tests/common/tester_query.h"

#include "workload/tpcc/tpcc_db.h"
#include "workload/tpch/tpch_db.h"


using namespace qpipe;
using namespace workload;
using namespace tpcc;


void wait_for_clients(pthread_t* thread_ids, int num_thread_ids);
void* start_client( void* ptr);


// FIXME (ip) Quick hack just to check if memory is important
#define NOS_CLIENTS      8
#define NOS_ITERATIONS 200

void tpcc_payment_nos_driver::submit(void* disp) {

    query_info_t info;
    info.num_clients    = NOS_CLIENTS;
    info.num_iterations = NOS_ITERATIONS;

    stopwatch_t timer;
    array_guard_t<pthread_t>client_ids = new pthread_t[info.num_clients];

    TRACE( TRACE_ALWAYS, "CL=%d IT=%d\n", info.num_clients, info.num_iterations);

    int err;
    pthread_attr_t pattr;


    try {
        for(int idx=0; idx < info.num_clients; idx++) {
  
            // create a new kernel schedulable thread
            err = pthread_attr_init( &pattr );
            THROW_IF(ThreadException, err);
            
            err = pthread_attr_setscope( &pattr, PTHREAD_SCOPE_SYSTEM );
            THROW_IF(ThreadException, err);
  
    
            tpcc_payment_single_thr_driver* client = 
                new tpcc_payment_single_thr_driver(c_str("PAYMENT_SINGLE_THR"), 
                                                   idx,
                                                   info.num_iterations);
  
            client_ids[idx] = thread_create(client);
        }
    }
    catch (QPipeException &e) {
        
        wait_for_clients(client_ids, info.num_clients);

        throw e;
    }    

    wait_for_clients(client_ids, info.num_clients);

    double etime = timer.time();

    //    TRACE(TRACE_STATISTICS, "Workload executed in\t (%.3lf) s\n", etime);
    TRACE(TRACE_STATISTICS, "\nThroughput \t(%.3lf) trx/sec\n", 
          (double)(info.num_clients * info.num_iterations)/etime);

}


void* start_client( void* ptr) {
    tpcc_payment_single_thr_driver* a_client = (tpcc_payment_single_thr_driver*)ptr;
    a_client->run();
    pthread_exit(0);
}


void wait_for_clients(pthread_t* thread_ids, int num_thread_ids)  {

    // wait for client threads to receive error message
    for (int i = 0; i < num_thread_ids; i++) {
        // join should not really fail unless we are doing
        // something seriously wrong...
        int join_ret = pthread_join(thread_ids[i], NULL);
        assert(join_ret == 0);
    }
}
