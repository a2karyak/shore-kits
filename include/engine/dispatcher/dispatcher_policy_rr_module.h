/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _DISPATCHER_POLICY_RR_MODULE_H
#define _DISPATCHER_POLICY_RR_MODULE_H

#include "engine/thread.h"
#include "engine/dispatcher/dispatcher_policy.h"
#include "engine/dispatcher/dispatcher_cpu_set_struct.h"
#include "engine/dispatcher/dispatcher_cpu_set.h"



// include me last!!!
#include "engine/namespace.h"



#define NUM_MODULES 2


/* exported datatypes */

class dispatcher_policy_rr_module_t : public dispatcher_policy_t {

protected:
    
    struct module_t {
        pthread_mutex_t _module_mutex;
        int _next_module_cpu;
    };

    pthread_mutex_t _next_module_mutex;
    int _next_module;

    struct dispatcher_cpu_set_s _cpu_set;
    module_t _modules[NUM_MODULES];
    int _cpus_per_module;

    class rr_module_query_state_t : public dispatcher_policy_t::query_state_t {
        
    public:
      
        int _module_index;

        rr_module_query_state_t(int module_index)
            : _module_index(module_index)
        {
        }
            
        virtual ~rr_module_query_state_t() { }
    };

public:
    
    dispatcher_policy_rr_module_t() {

        pthread_mutex_init_wrapper(&_next_module_mutex, NULL);
        _next_module = 0;
      
        int init_ret = dispatcher_cpu_set_init(&_cpu_set);
        assert( init_ret == 0 );

        for (int i = 0; i < NUM_MODULES; i++) {
            pthread_mutex_init_wrapper(&_modules[i]._module_mutex, NULL);
            _modules[i]._next_module_cpu = 0;
        }

        // Assume all modules have same number of CPUs and all cores
        // on the same module are stored adjacently within the cpu
        // set.
        _cpus_per_module = dispatcher_cpu_set_get_num_cpus(&_cpu_set) / NUM_MODULES;
    }


    virtual ~dispatcher_policy_rr_module_t() {
        dispatcher_cpu_set_finish(&_cpu_set);
        pthread_mutex_destroy_wrapper(&_next_module_mutex);

        for (int i = 0; i < NUM_MODULES; i++) {
            pthread_mutex_destroy_wrapper(&_modules[i]._module_mutex);
        }                                                 
    }
    
    
    virtual query_state_t* query_state_create() {
        int next_module;
    
        critical_section_t cs(&_next_module_mutex);
    
        // RR-MODULE dispatching policy requires that every new query
        // results in an increment of the next module index.
        next_module = _next_module;
        _next_module = (_next_module + 1) % NUM_MODULES;
        
        cs.exit();
    
        return new rr_module_query_state_t( next_module );
    }

  
    virtual void query_state_destroy(query_state_t* qs) {
        // Dynamic cast acts like an assert(), verifying the type.
        rr_module_query_state_t* qstate = dynamic_cast<rr_module_query_state_t*>(qs);
        delete qstate;
    }

  
    virtual void assign_packet_to_cpu(packet_t* packet, query_state_t* qs) {

        // Dynamic cast acts like an assert(), verifying the type.
        rr_module_query_state_t* qstate = dynamic_cast<rr_module_query_state_t*>(qs);
        int module_index = qstate->_module_index;
        module_t* module = &_modules[module_index];

        int next_cpu;
        critical_section_t cs(&module->_module_mutex);

        // RR_MODULE dispatching policy requires that every call to
        // assign_packet_to_cpu() results in an increment of the
        // module's next cpu index.
        next_cpu = module->_next_module_cpu;
        module->_next_module_cpu = (next_cpu + 1) % _cpus_per_module;
        
        cs.exit();
        
        packet->_bind_cpu = dispatcher_cpu_set_get_cpu( &_cpu_set, module_index * _cpus_per_module + next_cpu );
    }

};



#include "engine/namespace.h"



#endif
