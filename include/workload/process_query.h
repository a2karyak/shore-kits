/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _PROCESS_QUERY_H
#define _PROCESS_QUERY_H

#include "core/tuple.h"
#include "core/packet.h"

using namespace qpipe;


ENTER_NAMESPACE(workload);


/* exported datatypes */

class process_tuple_t {
public:
    virtual void begin() { }
    virtual void process(const tuple_t&)=0;
    virtual void end() { }
    virtual ~process_tuple_t() { }
};


void process_query(packet_t* root, process_tuple_t& pt);


EXIT_NAMESPACE(workload);


#endif
