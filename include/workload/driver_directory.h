/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _DRIVER_DIRECTORY_H
#define _DRIVER_DIRECTORY_H

#include "util.h"
#include "workload/driver.h"



ENTER_NAMESPACE(workload);



/**
 *  @class driver_directory_t
 *
 *  @brief Provides a lookup method that can be used to translate
 *  strings into drivers.
 */

class driver_directory_t {

public:

    virtual driver_t* lookup_driver(const c_str &tag)=0;
    
    virtual ~driver_directory_t() { }
};



EXIT_NAMESPACE(workload);



#endif // _DRIVER_DIRECTORY_H
