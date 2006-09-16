/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _POLICY_QUERY_CPU_H
#define _POLICY_QUERY_CPU_H

#include "util.h"
#include "scheduler/policy.h"
#include "scheduler/cpu_set_struct.h"
#include "scheduler/cpu_set.h"



ENTER_NAMESPACE(scheduler);



/* exported datatypes */

class policy_query_cpu_t : public policy_t {

protected:

    pthread_mutex_t _cpu_next_mutex;
    struct cpu_set_s _cpu_set;
    int _cpu_next;
    int _cpu_num;
 

    class query_cpu_query_state_t : public policy_t::query_state_t {

    public:

        cpu_t _qcpu;

        query_cpu_query_state_t(cpu_t qcpu)
            : _qcpu(qcpu)
        {
        }
        virtual ~query_cpu_query_state_t() { }
    };

  
public:
   
    policy_query_cpu_t()
        : _cpu_next_mutex(thread_mutex_create())
    {
        cpu_set_init(&_cpu_set);
        _cpu_next = 0;
        _cpu_num  = cpu_set_get_num_cpus(&_cpu_set);
    }


    virtual ~policy_query_cpu_t() {
        cpu_set_finish(&_cpu_set);
        thread_mutex_destroy(_cpu_next_mutex);
    }


    virtual query_state_t* query_state_create() {

        int next_cpu;
    
        critical_section_t cs(_cpu_next_mutex);
    
        // RR-CPU dispatching policy requires that every call to
        // assign_packet_to_cpu() results in an increment of the next cpu
        // index.
        next_cpu = _cpu_next;
        _cpu_next = (_cpu_next + 1) % _cpu_num;
    
        cs.exit();
    
        return new query_cpu_query_state_t( cpu_set_get_cpu(&_cpu_set, next_cpu) );
    }
  

    virtual void query_state_destroy(query_state_t* qs) {
        // Dynamic cast acts like an assert(), verifying the type.
        query_cpu_query_state_t* qstate = dynamic_cast<query_cpu_query_state_t*>(qs);
        delete qstate;
    }

    
    virtual void assign_packet_to_cpu(packet_t* packet, query_state_t* qs) {
        query_cpu_query_state_t* state = dynamic_cast<query_cpu_query_state_t*>(qs);
        packet->_cpu_bind = new policy_cpu_bind(state->_qcpu);
    }

};



EXIT_NAMESPACE(scheduler);



#endif
