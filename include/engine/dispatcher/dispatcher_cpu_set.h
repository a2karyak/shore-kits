/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file dispatcher_cpu_set.h
 *
 *  @brief Exports dispatcher_cpu_set_t datatype.
 *
 *  @bug See dispatcher_cpu_set.cpp.
 */
#ifndef _DISPATCHER_CPU_SET_H
#define _DISPATCHER_CPU_SET_H

#include "engine/dispatcher/dispatcher_os_support.h"
#include "engine/dispatcher/dispatcher_cpu.h"


/* exported datatypes */

typedef struct dispatcher_cpu_set_s* dispatcher_cpu_set_t;


/* exported functions */

int dispatcher_cpu_set_init(dispatcher_cpu_set_t cpu_set);
int dispatcher_cpu_set_get_num_cpus(dispatcher_cpu_set_t cpu_set);
dispatcher_cpu_t dispatcher_cpu_set_get_cpu(dispatcher_cpu_set_t cpu_set, int index);
void dispatcher_cpu_set_finish(dispatcher_cpu_set_t cpu_set);


#endif
