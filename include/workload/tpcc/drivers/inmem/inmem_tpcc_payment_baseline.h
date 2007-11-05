/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file inmem_tpcc_payment_baseline.h
 *
 *  @brief Declaration of the driver that submits INMEM_PAYMENT_BASELINE 
 *  transaction requests, according to the TPCC specification.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __INMEM_TPCC_PAYMENT_BASELINE_DRIVER_H
#define __INMEM_TPCC_PAYMENT_BASELINE_DRIVER_H


#include "workload/driver.h"
#include "workload/tpcc/drivers/tpcc_payment_common.h"
#include "stages/tpcc/inmem/inmem_payment_baseline.h"


using namespace qpipe;
using namespace tpcc;
using namespace tpcc_payment;


ENTER_NAMESPACE(workload);


class inmem_tpcc_payment_baseline_driver : public driver_t {

public:

    inmem_tpcc_payment_baseline_driver(const c_str& description)
        : driver_t(description)
    {
    }

    ~inmem_tpcc_payment_baseline_driver() { 
    }
    
    virtual void submit(void* disp, memObject_t* mem);

    trx_packet_t* create_inmem_payment_baseline_packet(const c_str& client_prefix,
                                                       tuple_fifo* bp_buffer,
                                                       tuple_filter_t* bp_filter,
                                                       scheduler::policy_t* dp,
                                                       int sf);
};


EXIT_NAMESPACE(workload);

#endif
