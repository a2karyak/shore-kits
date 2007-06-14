/* -*- mode:C++; c-basic-offset:4 -*- */

# include "stages/tpcc/payment_baseline.h"
# include "stages/tpcc/payment_begin.h"

# include "tests/common/tester_query.h"

# include "workload/common.h"
# include "workload/tpcc/drivers/tpcc_payment_baseline.h"

using namespace qpipe;


int main(int argc, char* argv[]) {

    query_info_t info = query_init(argc, argv, TRX_ENV);
    
    register_stage<payment_baseline_stage_t>(10);

    workload::tpcc_payment_baseline_driver driver(c_str("PAYMENT_BASELINE"));

    trace_set(TRACE_QUERY_RESULTS);

    query_main(info, &driver, TRX_ENV);
    return 0;
}
