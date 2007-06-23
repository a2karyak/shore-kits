/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __TPCC_PAYMENT_DRIVER_H
#define __TPCC_PAYMENT_DRIVER_H

# include "stages/tpcc/common/trx_packet.h"
# include "stages/tpcc/payment_begin.h"

# include "workload/driver.h"
# include "workload/driver_directory.h"

using namespace qpipe;


ENTER_NAMESPACE(workload);


class tpcc_payment_driver : public driver_t {

public:

    tpcc_payment_driver(const c_str& description)
        : driver_t(description)
    {
    }

    virtual void submit(void* disp);

    trx_packet_t* create_begin_payment_packet(const c_str& client_prefix,
                                              tuple_fifo* bp_buffer,
                                              tuple_filter_t* bp_filter,
                                              scheduler::policy_t* dp,
                                              int sf);

};

EXIT_NAMESPACE(workload);

#endif
