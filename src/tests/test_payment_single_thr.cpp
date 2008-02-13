/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file test_payment_single_thr.cpp
 *
 *  @brief Test running TPC-C PAYMENT_BASELINE transaction without staging
 *  with BDB underneath.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "workload/workload.h"
#include "workload/tpcc/drivers/bdb/tpcc_payment_single_thr.h"

#include "tests/common/tester_query.h"

#include "workload/tpcc/tpcc_db.h"
#include "workload/tpch/tpch_db.h"


using namespace qpipe;
using namespace workload;
using namespace tpcc;


void* start_client( void* ptr);


int main(int argc, char* argv[]) {

    //    TRACE_SET ( DEFAULT_TRACE_MASK | TRACE_DEBUG | TRACE_TRX_FLOW );
    TRACE_SET ( DEFAULT_TRACE_MASK );

    // initialized workload info and opens corresponding database
    //    query_info_t info;
    //    info.num_iterations = 10000;
    //    info.num_clients    = 10;

    query_info_t info = query_single_thr_init(argc, argv, TRX_BDB_ENV);

    stopwatch_t timer;
    array_guard_t<pthread_t>client_ids = new pthread_t[info.num_clients];

    TRACE( TRACE_DEBUG, "CL=%d IT=%d\n", 
           info.num_clients, info.num_iterations);

    try {
        for(int idx=0; idx < info.num_clients; idx++) {
    
            tpcc_payment_single_thr_driver* client = 
                new tpcc_payment_single_thr_driver(c_str("BDB-PAY-SINGLE-THR"), 
                                                   idx,
                                                   info.num_iterations);
  
            client_ids[idx] = thread_create(client);
        }
    }
    catch (QPipeException &e) {
        
        workload::workload_t::wait_for_clients(client_ids, info.num_clients);

        tpcc::db_close();
        return (-1);
    }    

    workload::workload_t::wait_for_clients(client_ids, info.num_clients);

    double etime = timer.time();

    TRACE(TRACE_STATISTICS, "Workload executed in\t (%.3lf) s\n", etime);
    TRACE(TRACE_STATISTICS, "Throughput          \t (%.3lf) trx/sec\n", 
          (double)(info.num_clients * info.num_iterations)/etime);

    tpcc::db_close();
    return (0);
}


void* start_client( void* ptr) {
    tpcc_payment_single_thr_driver* aclient = (tpcc_payment_single_thr_driver*)ptr;
    aclient->work();
    pthread_exit(0);

    return (NULL);
}

