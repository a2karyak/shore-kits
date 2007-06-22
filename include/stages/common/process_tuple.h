/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __PROCESS_TUPLE_H
#define __PROCESS_TUPLE_H

#include "core/tuple.h"
#include "core/packet.h"


ENTER_NAMESPACE(qpipe);


/* exported datatypes */

class process_tuple_t {
public:
    virtual void begin() { }
    virtual void process(const tuple_t&)=0;
    virtual void end() { }
    virtual ~process_tuple_t() { }
};


EXIT_NAMESPACE(qpipe);


#endif
