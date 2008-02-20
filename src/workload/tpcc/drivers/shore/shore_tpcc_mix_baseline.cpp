/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_tpcc_mix_baseline.cpp
 *
 *  @brief Implements client that submits SHORE_MIX_BASELINE transaction requests,
 *  according to the TPCC specification.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "scheduler.h"
#include "util.h"
#include "workload/common.h"
#include "workload/tpcc/drivers/shore/shore_tpcc_mix_baseline.h"


using namespace qpipe;
using namespace tpcc;


ENTER_NAMESPACE(workload);


void shore_tpcc_mix_baseline_driver::submit(void* disp, memObject_t*) 
{ 
    int this_type =  ShoreTPCCEnv::_atpccrndgen.random_xct_type(rand()%100);
    
    switch (this_type) {
    case XCT_NEW_ORDER:
        p_no_baseline_driver->submit(disp,NULL);  break;
    case XCT_PAYMENT:
        p_pay_baseline_driver->submit(disp,NULL);  break;
    case XCT_ORDER_STATUS:
        p_ordst_baseline_driver->submit(disp,NULL);  break;
    case XCT_DELIVERY:
        p_del_baseline_driver->submit(disp,NULL);  break;
    case XCT_STOCK_LEVEL:
        p_stlev_baseline_driver->submit(disp,NULL);  break;        
    }
}

EXIT_NAMESPACE(workload);
