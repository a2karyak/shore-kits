/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file cpu.h
 *
 *  @brief Exports cpu_t datatype. QPIPE worker threads may
 *  invoke cpu_bind_self() to bind themselves to the
 *  specified CPU.
 *
 *  @bug See cpu.cpp.
 */
#ifndef _CPU_H
#define _CPU_H

#include "util/namespace.h"

ENTER_NAMESPACE(scheduler);

/* exported datatypes */

typedef struct cpu_s* cpu_t;


/* exported functions */

void cpu_bind_self(cpu_t cpu);
int  cpu_get_unique_id(cpu_t cpu);

EXIT_NAMESPACE(scheduler);

#endif
