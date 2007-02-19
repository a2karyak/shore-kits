/* -*- mode:C++; c-basic-offset:4 -*- */

# include "stages/tpcc/payment_begin.h"

# include "tests/common/tester_query.h"

# include "workload/common.h"
# include "workload/tpcc/drivers/tpcc_payment.h"

using namespace qpipe;


int main(int argc, char* argv[]) {

    query_info_t info = query_init(argc, argv);
    
    register_stage<payment_begin_stage_t>(1);

    workload::tpcc_payment_driver driver(c_str("PAYMENT"));

    trace_set(TRACE_QUERY_RESULTS);

    query_main(info, &driver);
    return 0;
}
