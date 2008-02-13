/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_payment_baseline.h
 *
 *  @brief Declaration of the driver that submits PAYMENT_BASELINE transaction requests,
 *  according to the TPCC specification.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __BDB_TPCC_PAYMENT_BASELINE_DRIVER_H
#define __BDB_TPCC_PAYMENT_BASELINE_DRIVER_H


#include "workload/driver.h"

#include "stages/tpcc/common/tpcc_payment_common.h"
#include "stages/tpcc/bdb/payment_functions.h"
#include "stages/tpcc/bdb/payment_baseline.h"


using namespace qpipe;
using namespace tpcc;
using namespace tpcc_payment;


ENTER_NAMESPACE(workload);


class tpcc_payment_baseline_driver : public driver_t {

public:

    tpcc_payment_baseline_driver(const c_str& description)
        : driver_t(description)
    {
    }

    ~tpcc_payment_baseline_driver() { 
    }
    
    virtual void submit(void* disp, memObject_t* mem);

    bdb_trx_packet_t* create_payment_baseline_packet(const c_str& client_prefix,
                                                     tuple_fifo* bp_buffer,
                                                     tuple_filter_t* bp_filter,
                                                     scheduler::policy_t* dp,
                                                     s_payment_dbt_t* p_dbts,
                                                     int sf);
};


EXIT_NAMESPACE(workload);

#endif
