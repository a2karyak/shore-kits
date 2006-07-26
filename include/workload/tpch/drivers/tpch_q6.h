/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/driver.h"


using namespace qpipe;


class tpch_q6_driver : public driver_t {

public:

    tpch_q6_driver(const c_str& description)
        : driver_t(description)
    {
    }

    virtual void submit(void* disp);
    
};
