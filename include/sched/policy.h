// -*- mode:C++; c-basic-offset:4 -*-


#ifndef _POLICY_H
#define _POLICY_H

#include "core/packet.h"
#include "core/query_state.h"
#include "scheduler/cpu.h"



ENTER_NAMESPACE(scheduler);


using qpipe::packet_t;
using qpipe::query_state_t;


class policy_t {

public:

    virtual query_state_t* query_state_create()=0;
    virtual void query_state_destroy(query_state_t* qstate)=0;
    virtual ~policy_t() { }
};



EXIT_NAMESPACE(scheduler);



#endif
