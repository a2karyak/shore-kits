/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _DISPATCHER_POLICY_OS_H
#define _DISPATCHER_POLICY_OS_H

#include "engine/dispatcher/dispatcher_policy.h"
#include <cstdlib>



// include me last!!!
#include "engine/namespace.h"



/* exported datatypes */

class dispatcher_policy_os_t : public dispatcher_policy_t {

public:

    class os_query_state_t : public dispatcher_policy_t::query_state_t {

    protected:
        os_query_state_t() { }
        virtual ~os_query_state_t() { }
    };



    dispatcher_policy_os_t() { }


    virtual ~dispatcher_policy_os_t() {}

    
    virtual query_state_t* query_state_create() {
        // No state to maintina for OS policy. The value that we return
        // here should be opaquely passed to our own
        // assign_packet_to_cpu() method.
        return NULL;
    }


    virtual void query_state_destroy(query_state_t* qs) {
        // Nothing created... nothing to destroy
        assert( qs == NULL );
    }

  
    virtual void assign_packet_to_cpu(packet_t* packet, query_state_t* qs) {

        // error checking
        assert( qs == NULL );

        // OS dispatching policy requires that worker threads who pick up
        // packets never use hard affinity to (re-)bind themselves. We
        // indicate that no rebinding should take place by setting the
        // packet CPU to NULL.

        packet->_bind_cpu = NULL;
    }

};



#include "engine/namespace.h"



#endif
