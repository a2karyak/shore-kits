/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _DISPATCHER_GLOBALS_H
#define _DISPATCHER_GLOBALS_H

#include "scheduler/policy.h"

void global_scheduler_policy_set(const char* dpolicy);
scheduler::policy_t* global_scheduler_policy_get(void);

#endif
