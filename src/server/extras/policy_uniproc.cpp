/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"
#include "scheduler/policy.h"
#include "scheduler/cpu_set_struct.h"
#include "scheduler/cpu_set.h"


using namespace scheduler;


class policy_uniproc_t : public policy_t {

protected:

    class uniproc_query_state_t : public qpipe::query_state_t {
    public:
        uniproc_query_state_t() { }
        virtual ~uniproc_query_state_t() { }
    };        

    virtual cpu_t assign(packet_t*, query_state_t*) {
        cpu_set_s cpu_set;
        cpu_set_init(&cpu_set);
        return cpu_set_get_cpu(&cpu_set, 0);
    }

public:

    policy_uniproc_t() { }
    virtual ~policy_uniproc_t() { }


    virtual query_state_t* query_state_create() {
        return new uniproc_query_state_t();
    }


    virtual void query_state_destroy(query_state_t* qs) {
        uniproc_query_state_t* qstate =
            dynamic_cast<uniproc_query_state_t*>(qs);
        delete qstate;
    }

};

// hook for dlsym
extern "C"
policy_t* uniproc() {
    return new policy_uniproc_t();
}
