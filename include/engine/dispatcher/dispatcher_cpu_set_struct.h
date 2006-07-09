/* -*- mode:C++; c-basic-offset:4 -*- */


/** @file dispatcher_cpu_set_struct.h
 *
 *  @brief Exports dispatcher_cpu_set_s structure. Do not include this
 *  file unless a module needs to statically create these
 *  structures. All manipulations should be done on a
 *  dispatcher_cpu_set_t instance using one of the exported macros.
 *
 *  @bug See dispatcher_cpu.cpp.
 */
#ifndef _DISPATCHER_CPU_SET_STRUCT_H
#define _DISPATCHER_CPU_SET_STRUCT_H

#include "engine/dispatcher/dispatcher_os_support.h"
#include "engine/dispatcher/dispatcher_cpu_struct.h"



/* exported structures */


/**
 *  @brief A set of CPUs.
 */
struct dispatcher_cpu_set_s
{
  /** The number of CPUs in this set. */
  int cpuset_num_cpus;

  /** The CPUs in this set. */
  struct dispatcher_cpu_s* cpuset_cpus;
};



#endif
