/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef __QUERY_STATE_H
#define __QUERY_STATE_H

#include "util.h"


ENTER_NAMESPACE(qpipe);


class packet_t;
struct query_state_t {

    /* We expect this class to be subclassed. */

protected:
    query_state_t() { }
    virtual ~query_state_t() { }
    
public:

    void* operator new(size_t size);
    void operator delete(void* ptr);
    
    /**
     * We could create a default implementation that does not
     * rebinding, but this is safer.
     */
    virtual void rebind_self(packet_t*)=0;

};


EXIT_NAMESPACE(qpipe);


#endif 
