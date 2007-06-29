/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef __TPCH_M146_H
#define __TPCH_M146_H

#include "workload/driver.h"
#include "workload/driver_directory.h"


ENTER_NAMESPACE(workload);


class tpch_m146_driver : public driver_t {

private:

    driver_directory_t* _directory;

public:

    tpch_m146_driver(const c_str& description, driver_directory_t* directory)
        : driver_t(description),
          _directory(directory)
    {
    }
    
    virtual void submit(void* disp, memObject_t* mem);
    
};

EXIT_NAMESPACE(workload);

#endif
