// -*- mode:C++; c-basic-offset:4 -*-

#ifndef _Q6_PACKET_H
#define _Q6_PACKET_H


#include "core.h"
#include "scheduler.h"

using namespace qpipe;
using namespace scheduler;


packet_t* create_q6_packet(const c_str &client_prefix, policy_t* dp);


#endif
