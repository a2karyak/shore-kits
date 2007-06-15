/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_payment_baseline.cpp
 *
 *  @brief Declaration of the driver that submits PAYMENT_BASELINE transaction requests,
 *  according to the TPCC specification.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCC_PAYMENT_BASELINE_DRIVER_H
#define __TPCC_PAYMENT_BASELINE_DRIVER_H


# include "stages/tpcc/trx_packet.h"
# include "stages/tpcc/payment_baseline.h"

# include "workload/driver.h"
# include "workload/driver_directory.h"

using namespace qpipe;


ENTER_NAMESPACE(workload);


class tpcc_payment_baseline_driver : public driver_t {

public:

    tpcc_payment_baseline_driver(const c_str& description)
      : driver_t(description)
      {
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