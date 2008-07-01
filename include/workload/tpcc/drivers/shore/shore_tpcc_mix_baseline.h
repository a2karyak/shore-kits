/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_tpcc_mix_baseline.h
 *
 *  @brief Declaration of the driver that submits the TPC-C SHORE_TPCCMIX_BASELINE 
 *  transaction requests.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __SHORE_TPCC_MIX_BASELINE_DRIVER_H
#define __SHORE_TPCC_MIX_BASELINE_DRIVER_H


#include "workload/driver.h"
#include "workload/tpcc/drivers/shore/shore_tpcc_new_order_baseline.h"
#include "workload/tpcc/drivers/shore/shore_tpcc_payment_baseline.h"
#include "workload/tpcc/drivers/shore/shore_tpcc_order_status_baseline.h"
#include "workload/tpcc/drivers/shore/shore_tpcc_delivery_baseline.h"
#include "workload/tpcc/drivers/shore/shore_tpcc_stock_level_baseline.h"


// For random number generator (rng)
#include <boost/random.hpp>

using namespace qpipe;
using namespace tpcc;


ENTER_NAMESPACE(workload);


class shore_tpcc_mix_baseline_driver : public driver_t 
{
private:

    shore_tpcc_new_order_baseline_driver*    p_no_baseline_driver;
    shore_tpcc_payment_baseline_driver*      p_pay_baseline_driver;
    shore_tpcc_order_status_baseline_driver* p_ordst_baseline_driver;
    shore_tpcc_delivery_baseline_driver*     p_del_baseline_driver;
    shore_tpcc_stock_level_baseline_driver*  p_stlev_baseline_driver;

    boost::rand48 rng;

public:

    shore_tpcc_mix_baseline_driver(const c_str& description)
        : driver_t(description)
    {
        // initiates the drivers
        p_no_baseline_driver = new 
            shore_tpcc_new_order_baseline_driver(c_str("SHORE_MIX_NO_BASELINE"));
        p_pay_baseline_driver = new 
            shore_tpcc_payment_baseline_driver(c_str("SHORE_MIX_PAY_BASELINE"));
        p_ordst_baseline_driver = new 
            shore_tpcc_order_status_baseline_driver(c_str("SHORE_MIX_ORDST_BASELINE"));
        p_del_baseline_driver = new 
            shore_tpcc_delivery_baseline_driver(c_str("SHORE_MIX_DEL_BASELINE"));
        p_stlev_baseline_driver = new 
            shore_tpcc_stock_level_baseline_driver(c_str("SHORE_MIX_STLEV_BASELINE"));
    }

    ~shore_tpcc_mix_baseline_driver() 
    { 
        if (p_no_baseline_driver)
            delete (p_no_baseline_driver);
        if (p_pay_baseline_driver)
            delete (p_pay_baseline_driver);
        if (p_ordst_baseline_driver)
            delete (p_ordst_baseline_driver);
        if (p_del_baseline_driver)
            delete (p_del_baseline_driver);
        if (p_stlev_baseline_driver)
            delete (p_stlev_baseline_driver);
    }
    
    virtual void submit(void* disp);
};


EXIT_NAMESPACE(workload);

#endif
