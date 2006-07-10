/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _DISPATCHER_GLOBALS_H
#define _DISPATCHER_GLOBALS_H

#include "engine/dispatcher/dispatcher_policy.h"

void global_dispatcher_policy_set(const char* dpolicy);
qpipe::dispatcher_policy_t* global_dispatcher_policy_get(void);

#endif
