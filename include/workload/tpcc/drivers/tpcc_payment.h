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

# include "stages/tpcc/common/trx_packet.h"
# include "stages/tpcc/payment_begin.h"

# include "workload/driver.h"
# include "workload/driver_directory.h"

using namespace qpipe;


ENTER_NAMESPACE(workload);


class tpcc_payment_driver : public driver_t {

private:
    // whRange governs the range of warehouses queried by the client
    // the default value is RANGE (see trx_packet.h)
    int whRange;

public:

    tpcc_payment_driver(const c_str& description, const int aRange = RANGE)
        : driver_t(description)
    {
          assert (aRange > 0);
          whRange = aRange;
    }

    virtual void submit(void* disp);

    inline void setRange(const int aRange) {
        assert (aRange > 0);
        whRange = aRange;
    }

    trx_packet_t* create_begin_payment_packet(const c_str& client_prefix,
                                              tuple_fifo* bp_buffer,
                                              tuple_filter_t* bp_filter,
                                              scheduler::policy_t* dp,
                                              int sf);

};

EXIT_NAMESPACE(workload);

#endif
