/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages/tpcc/payment_baseline.h"
//# include "stages/tpcc/payment_begin.h"

#include "tests/common/tester_query.h"

#include "workload/common.h"
#include "workload/tpcc/drivers/tpcc_payment_baseline.h"


using namespace qpipe;

// sets certain range in the queried warehouses
const int WH = 4; 

int main(int argc, char* argv[]) {

    query_info_t info = query_init(argc, argv, TRX_ENV);
    
    register_stage<payment_baseline_stage_t>(10);

    workload::tpcc_payment_baseline_driver driver(c_str("PAYMENT_BASELINE"), WH);

    trace_set(TRACE_QUERY_RESULTS);

    //////
    /* testing the URand()
    scheduler::policy_t* dp = new scheduler::policy_os_t();
    for (int i=0; i<100; i++) {
        
        driver.create_payment_baseline_packet("kati", 
                                              new tuple_fifo(1), 
                                              new trivial_filter_t(1),
                                              dp,
                                              WH);
    }

    return (-1);
    /////
    //    EOF URand() testing 
    */

    query_main(info, &driver, TRX_ENV);
    return 0;
}
