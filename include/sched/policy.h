// -*- mode:C++; c-basic-offset:4 -*-


#ifndef _POLICY_H
#define _POLICY_H

#include "core/packet.h"


ENTER_NAMESPACE(scheduler);

using qpipe::packet_t;

struct policy_cpu_bind : qpipe::cpu_bind {
    cpu_t _cpu;
    policy_cpu_bind(cpu_t cpu)
        : _cpu(cpu)
    {
    }
    virtual void bind(packet_t*) {
        cpu_bind_self(_cpu);
    }
};

class policy_t {

public:
  
    class query_state_t {
    
    protected:

        query_state_t() { }
        virtual ~query_state_t() { }
    
    };


    virtual query_state_t* query_state_create()=0;
    virtual void query_state_destroy(query_state_t* qs)=0;
    virtual void assign_packet_to_cpu(packet_t* packet, query_state_t* qs)=0;
    
};



EXIT_NAMESPACE(scheduler);



#endif
