/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file test_payment_baseline.cpp
 *
 *  @brief Test running TPC-C PAYMENT_BASELINE transaction with BDB underneath. 
 *  We need to register only one stage - payment_baseline_stage_t
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "stages/tpcc/bdb/payment_baseline.h"

#include "tests/common/tester_query.h"

#include "workload/common.h"
#include "workload/tpcc/drivers/bdb/tpcc_payment_baseline.h"


using namespace qpipe;

int main(int argc, char* argv[]) {

    query_info_t info = query_init(argc, argv, TRX_BDB_ENV);
    
    register_stage<payment_baseline_stage_t>(10);

    workload::tpcc_payment_baseline_driver driver(c_str("BDB-PAY-BASELINE"));

    trace_set(TRACE_QUERY_RESULTS);

    query_main(info, &driver, TRX_BDB_ENV);
    return 0;
}
