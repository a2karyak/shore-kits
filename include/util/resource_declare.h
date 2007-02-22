/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __RESOURCE_DECLARE_H
#define __RESOURCE_DECLARE_H

#include "util/c_str.h"


/* exported datatypes */


/**
 *  @brief Provide a way for packets to declare how many resources
 *  they need. Used to avoid deadlocks by imposing total ordering when
 *  we acquire the resources.
 */

class resource_declare_t
{
public:
    virtual void declare(const c_str& name, int count)=0;
    virtual ~resource_declare_t() { }
};


#endif
