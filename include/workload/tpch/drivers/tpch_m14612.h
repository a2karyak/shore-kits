/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __TPCH_M14612_H
#define __TPCH_M14612_H

#include "workload/driver.h"
#include "workload/driver_directory.h"


ENTER_NAMESPACE(workload);


class tpch_m14612_driver : public driver_t {

private:

    driver_directory_t* _directory;

public:

    tpch_m14612_driver(const c_str& description, driver_directory_t* directory)
        : driver_t(description),
          _directory(directory)
    {
    }
    
    virtual void submit(void* disp, memObject_t* mem);    
};

EXIT_NAMESPACE(workload);

#endif
