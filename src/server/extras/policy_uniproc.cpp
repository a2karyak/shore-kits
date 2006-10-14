/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"
#include "scheduler/policy.h"
#include "scheduler/cpu_set_struct.h"
#include "scheduler/cpu_set.h"

using namespace scheduler;
class policy_uniproc_t : public policy_t {
    virtual query_state_t* query_state_create() {
        return NULL;
    }
    virtual void query_state_destroy(query_state_t*) { }
    virtual void assign_packet_to_cpu(packet_t* packet, query_state_t*) {
        cpu_set_s cpu_set;
        cpu_set_init(&cpu_set);
        packet->_cpu_bind = new policy_cpu_bind(cpu_set_get_cpu(&cpu_set, 0));
    }
};

// hook for dlsym
extern "C"
policy_t* uniproc() {
    return new policy_uniproc_t();
}
