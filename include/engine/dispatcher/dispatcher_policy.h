// -*- mode:C++; c-basic-offset:4 -*-


#ifndef _DISPATCHER_POLICY_H
#define _DISPATCHER_POLICY_H

#include "engine/core/packet.h"



// include me last!!!
#include "engine/namespace.h"



class dispatcher_policy_t {

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



#include "engine/namespace.h"



#endif
