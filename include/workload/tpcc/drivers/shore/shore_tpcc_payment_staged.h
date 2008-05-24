/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_tpcc_payment_staged.h
 *
 *  @brief Declaration of the driver that submits SHORE_PAYMENT_STAGED 
 *  transaction requests, according to the TPCC specification.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __SHORE_TPCC_PAYMENT_STAGED_DRIVER_H
#define __SHORE_TPCC_PAYMENT_STAGED_DRIVER_H


#include "workload/driver.h"
#include "stages/tpcc/common/tpcc_trx_input.h"
#include "stages/tpcc/shore/staged/shore_payment_begin.h"


using namespace qpipe;
using namespace tpcc;


ENTER_NAMESPACE(workload);


class shore_tpcc_payment_staged_driver : public driver_t {

public:

    shore_tpcc_payment_staged_driver(const c_str& description)
        : driver_t(description)
    {
    }

    ~shore_tpcc_payment_staged_driver() { 
    }
    
    virtual void submit(void* disp);

    trx_packet_t* create_shore_payment_staged_packet(tuple_fifo* bp_buffer,
                                                     tuple_filter_t* bp_filter,
                                                     int sf);
};


EXIT_NAMESPACE(workload);

#endif
