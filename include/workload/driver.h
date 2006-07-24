/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _WORKLOAD_DRIVER_H
#define _WORKLOAD_DRIVER_H

#include "engine/util/c_str.h"



#include "engine/namespace.h"



/**
 *  @class driver_t
 *
 *  @brief A driver is basically a wrapper around a submit() method
 *  that clients can invoke to submit requests into the system. This
 *  lets us isolate all workload specific properties in a single
 *  class. One important point is that driver_t instances will be
 *  shared by concurrent workload client threads. Because of this, a
 *  driver SHOULD NOT store any object state. The submit() function
 *  should allocate all required data structures within the context of
 *  the calling thread.
 */

class driver_t {

private:
    
    c_str _description;
    
public:

    driver_t(const c_str& description)
        : _description(description)
    {
    }

    virtual ~driver_t() { }

    virtual void submit(void* arg)=0;
};



#include "engine/namespace.h"



#endif // _DRIVER_H
