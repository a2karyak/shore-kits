// -*- mode:C++; c-basic-offset:4 -*-

#ifndef _Q6_PACKET_H
#define _Q6_PACKET_H


#include "engine/core/packet.h"
#include "engine/dispatcher/dispatcher_policy.h"

using namespace qpipe;


packet_t* create_q6_packet(const c_str &client_prefix, dispatcher_policy_t* dp);


#endif
