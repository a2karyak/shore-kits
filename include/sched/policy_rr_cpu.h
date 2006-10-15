/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _POLICY_RR_CPU_H
#define _POLICY_RR_CPU_H

#include "util.h"
#include "scheduler/policy.h"
#include "scheduler/cpu_set_struct.h"
#include "scheduler/cpu_set.h"



ENTER_NAMESPACE(scheduler);



/* exported datatypes */

class policy_rr_cpu_t : public policy_t {

protected:

    class rr_cpu_state_t : public qpipe::query_state_t {
    public:
        rr_cpu_state_t() { }
        virtual ~rr_cpu_state_t() { }
    };


    pthread_mutex_t _cpu_next_mutex;
    struct cpu_set_s _cpu_set;
    int _cpu_next;
    int _cpu_num;
 
    virtual cpu_t assign(packet_t*, query_state_t*) {

        int next_cpu;

        critical_section_t cs(_cpu_next_mutex);

        // RR-CPU dispatching policy requires that every call to
        // assign_packet_to_cpu() results in an increment of the next cpu
        // index.
        next_cpu = _cpu_next;
        _cpu_next = (_cpu_next + 1) % _cpu_num;

        cs.exit();

        return cpu_set_get_cpu( &_cpu_set, next_cpu );
    }


public:

    policy_rr_cpu_t()
        : _cpu_next_mutex(thread_mutex_create())
    {
        cpu_set_init(&_cpu_set);
        _cpu_next = 0;
        _cpu_num  = cpu_set_get_num_cpus(&_cpu_set);
    }


    virtual ~policy_rr_cpu_t() {
        cpu_set_finish(&_cpu_set);
        thread_mutex_destroy(_cpu_next_mutex);
    }


    virtual query_state_t* query_state_create() {
        return new rr_cpu_state_t();
    }

  
    virtual void query_state_destroy(query_state_t* qs) {
        rr_cpu_state_t* qstate = dynamic_cast<rr_cpu_state_t*>(qs);
        delete qstate;
    }


};



EXIT_NAMESPACE(scheduler);



#endif
