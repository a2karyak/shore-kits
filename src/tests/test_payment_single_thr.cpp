/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file test_payment_single_thr.cpp
 *
 *  @brief Test running TPC-C PAYMENT_BASELINE transaction without staging
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "workload/workload.h"
#include "workload/tpcc/drivers/tpcc_payment_single_thr.h"

#include "tests/common/tester_query.h"

#include "workload/tpcc/tpcc_db.h"
#include "workload/tpch/tpch_db.h"


using namespace qpipe;
using namespace workload;


int main(int argc, char* argv[]) {

    TRACE_SET ( DEFAULT_TRACE_MASK );

    // initialized workload info and opens corresponding database
    query_info_t info = query_single_thr_init(argc, argv, TRX_ENV);    
    stopwatch_t timer;
    array_guard_t<pthread_t>client_ids = new pthread_t[info.num_clients];

    try {
        for(int idx=0; idx < info.num_clients; idx++) {
    
            tpcc_payment_single_thr_driver client(c_str("PAYMENT_SINGLE_THR"), idx);
            client_ids[idx] = thread_create(&client);
        }
        
        TRACE(TRACE_STATISTICS, "Query executed in %.3lf s\n", timer.time());
    }
    catch (QPipeException &e) {
        
        workload_t::wait_for_clients(client_ids, info.num_clients);

        tpcc::db_close();
        return (-1);
    }    

    tpcc::db_close();
    return (0);
}
