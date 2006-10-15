/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef __QUERY_STATE_H
#define __QUERY_STATE_H

#include "util.h"


ENTER_NAMESPACE(qpipe);


struct query_state_t {

    /* We expect this class to be subclassed. */

protected:
    query_state_t() { }
    virtual ~query_state_t() { }
};


EXIT_NAMESPACE(qpipe);


#endif 
