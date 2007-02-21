/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __RESOURCE_RESERVER_H
#define __RESOURCE_RESERVER_H

#include "util/c_str.h"


/* exported datatypes */


/**
 *  @brief An interface to separate the declaration of resource needs
 *  from resource acquisition. Basically, a way to avoid deadlocks
 *  when acquiring resources.
 */

class resource_reserver_t
{
public:
    virtual void declare_resource_need(const c_str& name, int count)=0;
    virtual void acquire_resources()=0;
    virtual ~resource_reserver_t() { }
};


#endif
