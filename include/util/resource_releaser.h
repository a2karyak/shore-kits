/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __RESOURCE_RELEASER_H
#define __RESOURCE_RELEASER_H

/* exported datatypes */

/**
 *  @brief Provide a way for packets to declare how many resources
 *  they need. Used to avoid deadlocks by imposing total ordering when
 *  we acquire the resources.
 */

class resource_releaser_t
{
public:
    virtual void release_resources()=0;
    virtual ~resource_releaser_t() { }
};


#endif
