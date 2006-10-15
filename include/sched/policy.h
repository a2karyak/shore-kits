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

protected:

    virtual cpu_t assign(packet_t* packet, query_state_t* qstate)=0;

public:

    virtual query_state_t* query_state_create()=0;
    virtual void query_state_destroy(query_state_t* qstate)=0;

    cpu_t assign_packet_to_cpu(packet_t* packet) {

        assert(packet != NULL);

        /* Sometimes, we may want to dispatch a packet without CPU
           binding. One way to do this is to set the query state of
           the packet to NULL. We check this corner case here so that
           subclasses don't have to worry about it. */
        query_state_t* qstate = packet->get_query_state();
        if (qstate == NULL) {
            /* no CPU binding for this packet... */
            return NULL;
        }

        /* assign() can now assume that qstate is not NULL */
        return assign(packet, qstate);
    }

};



EXIT_NAMESPACE(scheduler);



#endif
