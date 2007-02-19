/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __TPCC_PAYMENT_H
#define __TPCC_PAYMENT_H

# include "stages/tpcc/trx_packet.h"
# include "workload/driver.h"
# include "core.h"
# include "scheduler.h"

using namespace qpipe;


ENTER_NAMESPACE(workload);


class tpcc_payment_driver : public driver_t {

public:

    tpcc_payment_driver(const c_str& description)
        : driver_t(description)
    {
    }

    virtual void submit(void* disp);

    trx_packet_t* create_payment_packet(const c_str& client_prefix,
                                        scheduler::policy_t* dp);

};

EXIT_NAMESPACE(workload);

#endif
