/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef __TPCH_SIM_MIX_H
#define __TPCH_SIM_MIX_H

#include "workload/driver.h"
#include "workload/driver_directory.h"


ENTER_NAMESPACE(workload);


class tpch_sim_mix_driver : public driver_t {

private:

    driver_directory_t* _directory;

public:

    tpch_sim_mix_driver(const c_str& description, driver_directory_t* directory)
        : driver_t(description),
          _directory(directory)
    {
    }
    
    virtual void submit(void* disp);
    
};

EXIT_NAMESPACE(workload);

#endif
