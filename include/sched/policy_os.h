/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _POLICY_OS_H
#define _POLICY_OS_H

#include "scheduler/policy.h"
#include <cstdlib>



ENTER_NAMESPACE(scheduler);



/* exported datatypes */

class policy_os_t : public policy_t {

protected:

    class os_query_state_t : public qpipe::query_state_t {
    public:
        os_query_state_t() { }
        virtual ~os_query_state_t() { }
    };


    virtual cpu_t assign(packet_t*, query_state_t*) {
        // OS dispatching policy requires that worker threads who pick up
        // packets never use hard affinity to (re-)bind themselves. We
        // indicate that no rebinding should take place by returning a CPU of NULL.
        return NULL;
    }


public:

    policy_os_t() { }
    virtual ~policy_os_t() {}

    
    virtual query_state_t* query_state_create() {
        return new os_query_state_t();
    }


    virtual void query_state_destroy(query_state_t* qs) {
        os_query_state_t* qstate = dynamic_cast<os_query_state_t*>(qs);
        delete qstate;
    }

};



EXIT_NAMESPACE(scheduler);



#endif
