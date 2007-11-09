/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_payment.cpp
 *
 *  @brief Declaration of the client that submits PAYMENT transactions 
 *  according to the TPC-C specification.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCC_PAYMENT_DRIVER_H
#define __TPCC_PAYMENT_DRIVER_H


#include "workload/driver.h"

#include "workload/tpcc/drivers/tpcc_payment_common.h"
#include "stages/tpcc/payment_begin.h"


using namespace qpipe;
using namespace tpcc;


ENTER_NAMESPACE(workload);


class tpcc_payment_driver : public driver_t {

public:

    tpcc_payment_driver(const c_str& description)
        : driver_t(description)
    {
    }

    virtual ~tpcc_payment_driver() { }

    virtual void submit(void* disp, memObject_t* mem);

    bdb_trx_packet_t* create_begin_payment_packet(const c_str& client_prefix,
                                                  tuple_fifo* bp_buffer,
                                                  tuple_filter_t* bp_filter,
                                                  int sf);

};

EXIT_NAMESPACE(workload);

#endif
