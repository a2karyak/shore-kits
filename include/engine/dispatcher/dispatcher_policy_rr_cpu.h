/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _DISPATCHER_POLICY_RR_CPU_H
#define _DISPATCHER_POLICY_RR_CPU_H

#include "engine/thread.h"
#include "engine/dispatcher/dispatcher_policy.h"
#include "engine/dispatcher/dispatcher_cpu_set_struct.h"
#include "engine/dispatcher/dispatcher_cpu_set.h"



// include me last!!!
#include "engine/namespace.h"



/* exported datatypes */

class dispatcher_policy_rr_cpu_t : public dispatcher_policy_t {

protected:

    pthread_mutex_t _cpu_next_mutex;
    struct dispatcher_cpu_set_s _cpu_set;
    int _cpu_next;
    int _cpu_num;
 
public:

    class rr_cpu_query_state_t : public dispatcher_policy_t::query_state_t {

    protected:
        rr_cpu_query_state_t() { }
        virtual ~rr_cpu_query_state_t() { }
    };

    
    dispatcher_policy_rr_cpu_t() {
        pthread_mutex_init_wrapper(&_cpu_next_mutex, NULL);

        int init_ret = dispatcher_cpu_set_init(&_cpu_set);
        assert( init_ret == 0 );

        _cpu_next = 0;
        _cpu_num  = dispatcher_cpu_set_get_num_cpus(&_cpu_set);
    }


    virtual ~dispatcher_policy_rr_cpu_t() {
        dispatcher_cpu_set_finish(&_cpu_set);
        pthread_mutex_destroy_wrapper(&_cpu_next_mutex);
    }


    virtual query_state_t* query_state_create() {
        // No state to maintina for RR-CPU policy. The value that we
        // return here should be opaquely passed to our own
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

        int next_cpu;

        critical_section_t cs(&_cpu_next_mutex);

        // RR-CPU dispatching policy requires that every call to
        // assign_packet_to_cpu() results in an increment of the next cpu
        // index.
        next_cpu = _cpu_next;
        _cpu_next = (_cpu_next + 1) % _cpu_num;

        cs.exit();

        packet->_bind_cpu = dispatcher_cpu_set_get_cpu( &_cpu_set, next_cpu );
    }

};



#include "engine/namespace.h"



#endif
