/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file inmem_test_payment_single_thr.cpp
 *
 *  @brief Test running TPC-C PAYMENT_BASELINE transaction without staging
 *  using the InMemory data structures.
 *
 *  @note Since every time it was to load the database in-memory we execute
 *  each workload 5 times.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "workload/workload.h"
#include "workload/tpcc/drivers/inmem/inmem_tpcc_payment_single_thr.h"

#include "tests/common/tester_query.h"


using namespace qpipe;
using namespace workload;
using namespace tpcc;


const int ITERATIONS = 5;


void* start_client( void* ptr);
int do_iteration(int argc, char* argv[]);


int main(int argc, char* argv[]) {

    // Input validation. If incorrect input, aborts.
    query_info_t info = query_single_thr_init(argc, argv, TRX_MEM_ENV);


    //    TRACE_SET ( DEFAULT_TRACE_MASK | TRACE_DEBUG | TRACE_TRX_FLOW );
    TRACE_SET ( DEFAULT_TRACE_MASK );


    // Create the InMem Environment
    inmem_env = new InMemTPCCEnv();
    
    // Restores tpcc data
    //inmem_env->loaddata(INMEM_TPCC_DATA_DIR);
    inmem_env->restoredata(INMEM_TPCC_SAVE_DIR);
        

    for (int i=0; i<ITERATIONS; i++) {

        do_iteration(argc, argv);
    }

    delete info._policy;
    delete inmem_env;

    return (0);
}



int do_iteration(int argc, char* argv[]) {

    query_info_t info = query_single_thr_init(argc, argv, TRX_MEM_ENV);

    stopwatch_t timer;
    array_guard_t<pthread_t>client_ids = new pthread_t[info.num_clients];

    TRACE( TRACE_DEBUG, "CL=%d IT=%d SF=%d\n", 
           info.num_clients, info.num_iterations, info.scaling_factor);

    try {
        for(int idx=0; idx < info.num_clients; idx++) {
    
            inmem_tpcc_payment_single_thr_driver* client = 
                new inmem_tpcc_payment_single_thr_driver(c_str("INMEM-PAY-SINGLE-THR"), 
                                                         idx,
                                                         info.num_iterations,
                                                         info.scaling_factor,
                                                         inmem_env);
  
            client_ids[idx] = thread_create(client);
        }
    }
    catch (QPipeException &e) {

        TRACE( TRACE_ALWAYS, "Exception reached root level. Description:\n" \
               "%s\n", e.what());
        
        workload::workload_t::wait_for_clients(client_ids, info.num_clients);

        return (-1);
    }    

    workload::workload_t::wait_for_clients(client_ids, info.num_clients);

    double etime = timer.time();

    TRACE(TRACE_STATISTICS, "Workload executed in\t (%.3lf) s\n", etime);
    TRACE(TRACE_STATISTICS, "Throughput          \t (%.3lf) trx/sec\n", 
          (double)(info.num_clients * info.num_iterations)/etime);

    return (0);
}


void* start_client( void* ptr) {
    inmem_tpcc_payment_single_thr_driver* aclient = (inmem_tpcc_payment_single_thr_driver*)ptr;
    aclient->work();
    pthread_exit(0);

    return (NULL);
}

