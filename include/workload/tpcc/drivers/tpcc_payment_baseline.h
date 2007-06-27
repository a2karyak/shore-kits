/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_payment_baseline.h
 *
 *  @brief Declaration of the driver that submits PAYMENT_BASELINE transaction requests,
 *  according to the TPCC specification.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCC_PAYMENT_BASELINE_DRIVER_H
#define __TPCC_PAYMENT_BASELINE_DRIVER_H


#include "workload/tpcc/drivers/trx_driver.h"
#include "workload/tpcc/drivers/tpcc_payment_common.h"
#include "stages/tpcc/payment_baseline.h"


using namespace qpipe;
using namespace tpcc;


ENTER_NAMESPACE(workload);


// FIXME *******************************************
// FIXME (ip) The drivers should not save any state
// FIXME (ip) s_payment_dbt_t should be removed
// FIXME ******************************************


class tpcc_payment_baseline_driver : public trx_driver_t {

protected:

    // Structure for allocating only once all the Dbts
    s_payment_dbt_t _dbts;

    virtual void allocate_dbts();
    virtual void deallocate_dbts();
    virtual void reset_dbts();

public:

    tpcc_payment_baseline_driver(const c_str& description, const int aRange = RANGE)
        : trx_driver_t(description)
    {
        assert (aRange > 0);
        _whRange = aRange;
        
        allocate_dbts();
    }

    ~tpcc_payment_baseline_driver() { 
        deallocate_dbts();
    }
    
    virtual void submit(void* disp);

    trx_packet_t* create_payment_baseline_packet(const c_str& client_prefix,
                                                 tuple_fifo* bp_buffer,
                                                 tuple_filter_t* bp_filter,
                                                 scheduler::policy_t* dp,
                                                 int sf);
};


EXIT_NAMESPACE(workload);

#endif
