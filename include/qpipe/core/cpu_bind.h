/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef __CPU_BIND_H
#define __CPU_BIND_H

#include "util.h"

ENTER_NAMESPACE(qpipe);

class packet_t;

struct cpu_bind {
    struct nop;
public:
    // instructs the scheduler to choose which CPU (if any) to bind
    // the current thread to, based on the packet supplied
    virtual void bind(packet_t* packet)=0;
    virtual ~cpu_bind() { }
};

struct cpu_bind::nop : cpu_bind {
    virtual void bind(packet_t*) {
        // do nothing!
    }
};

EXIT_NAMESPACE(qpipe);

#endif 
