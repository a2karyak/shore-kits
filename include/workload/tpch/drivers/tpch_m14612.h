/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/driver.h"
#include "workload/driver_directory.h"


using namespace qpipe;


class tpch_m14612_driver : public driver_t {

private:

    driver_directory_t* _directory;

public:

    tpch_m14612_driver(const c_str& description, driver_directory_t* directory)
        : driver_t(description),
          _directory(directory)
    {
    }
    
    virtual void submit(void* disp);
    
};
