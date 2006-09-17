/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef __TPCH_Q6_H
#define __TPCH_Q6_H

#include "workload/driver.h"


ENTER_NAMESPACE(workload);


class tpch_q6_driver : public driver_t {

public:

    tpch_q6_driver(const c_str& description)
        : driver_t(description)
    {
    }

    virtual void submit(void* disp);
    
};

EXIT_NAMESPACE(workload);

#endif
