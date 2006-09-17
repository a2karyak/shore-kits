/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef __TPCH_Q1_H
#define __TPCH_Q1_H

#include "util.h"
#include "workload/driver.h"

ENTER_NAMESPACE(workload);

class tpch_q1_driver : public driver_t {

public:

    tpch_q1_driver(const c_str& description)
        : driver_t(description)
    {
    }

    virtual void submit(void* disp);
    
};

EXIT_NAMESPACE(workload);

#endif
