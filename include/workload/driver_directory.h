/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _DRIVER_DIRECTORY_H
#define _DRIVER_DIRECTORY_H

#include "engine/util/c_str.h"
#include "workload/driver.h"



#include "engine/namespace.h"



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



#include "engine/namespace.h"



#endif // _DRIVER_DIRECTORY_H
