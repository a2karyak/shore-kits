/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_payment_baseline.h
 *
 *  @brief Declaration of the driver that executes PAYMENT_BASELINE transactions
 *  without employing the staging mechanism.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCC_PAYMENT_NOS_DRIVER_H
#define __TPCC_PAYMENT_NOS_DRIVER_H


#include "workload/tpcc/drivers/trx_driver.h"
#include "workload/tpcc/drivers/tpcc_payment_common.h"
#include "workload/tpcc/drivers/tpcc_payment_nos.h"


using namespace qpipe;
using namespace tpcc;


ENTER_NAMESPACE(workload);


class tpcc_payment_nos_driver : public trx_driver_t {

protected:

    // Structure for allocating only once all the Dbts
    s_payment_dbt_t _dbts;

    virtual void allocate_dbts() { }
    virtual void deallocate_dbts() { }
    virtual void reset_dbts() { }

public:

    tpcc_payment_nos_driver(const c_str& description, const int aRange = RANGE)
        : trx_driver_t(description)
    {
        assert (aRange > 0);
        _whRange = aRange;
        
        allocate_dbts();
    }

    ~tpcc_payment_nos_driver() { 
        deallocate_dbts();
    }
    
    virtual void submit(void* disp);
};


EXIT_NAMESPACE(workload);

#endif
