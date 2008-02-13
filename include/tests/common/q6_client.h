// -*- mode:C++; c-basic-offset:4 -*-

#ifndef _Q6_CLIENT_H
#define _Q6_CLIENT_H

#include "scheduler.h"



// exported datatypes

struct q6_client_info_s {
    int _client_id;
    int _num_iterations;
    scheduler::policy_t* _policy;

    q6_client_info_s(int client_id, int num_iterations, scheduler::policy_t* policy)
        : _client_id(client_id),
          _num_iterations(num_iterations),
          _policy(policy)
    {
    }
};


void q6_client_main(void* arg);


#endif
