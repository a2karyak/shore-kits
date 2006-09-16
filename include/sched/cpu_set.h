/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file cpu_set.h
 *
 *  @brief Exports cpu_set_p datatype.
 *
 *  @bug See cpu_set.cpp.
 */
#ifndef _CPU_SET_H
#define _CPU_SET_H

#include "util/namespace.h"
#include "scheduler/os_support.h"
#include "scheduler/cpu.h"

ENTER_NAMESPACE(scheduler);
/* exported datatypes */

typedef struct cpu_set_s* cpu_set_p;

/* exported functions */

void cpu_set_init(cpu_set_p cpu_set);
int cpu_set_get_num_cpus(cpu_set_p cpu_set);
cpu_t cpu_set_get_cpu(cpu_set_p cpu_set, int index);
void cpu_set_finish(cpu_set_p cpu_set);

EXIT_NAMESPACE(scheduler);

#endif
