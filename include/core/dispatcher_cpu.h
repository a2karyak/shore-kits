/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file dispatcher_cpu.h
 *
 *  @brief Exports dispatcher_cpu_t datatype. QPIPE worker threads may
 *  invoke dispatcher_cpu_bind_self() to bind themselves to the
 *  specified CPU.
 *
 *  @bug See dispatcher_cpu.cpp.
 */
#ifndef _DISPATCHER_CPU_H
#define _DISPATCHER_CPU_H


/* exported datatypes */

typedef struct dispatcher_cpu_s* dispatcher_cpu_t;


/* exported functions */

void dispatcher_cpu_bind_self(dispatcher_cpu_t cpu);
int  dispatcher_cpu_get_unique_id(dispatcher_cpu_t cpu);


#endif
