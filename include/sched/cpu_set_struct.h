/* -*- mode:C++; c-basic-offset:4 -*- */


/** @file cpu_set_struct.h
 *
 *  @brief Exports cpu_set_s structure. Do not include this
 *  file unless a module needs to statically create these
 *  structures. All manipulations should be done on a
 *  cpu_set_p instance using one of the exported macros.
 *
 *  @bug See cpu.cpp.
 */
#ifndef _CPU_SET_STRUCT_H
#define _CPU_SET_STRUCT_H

#include "util/namespace.h"
#include "scheduler/os_support.h"
#include "scheduler/cpu_struct.h"

ENTER_NAMESPACE(scheduler);

/* exported structures */


/**
 *  @brief A set of CPUs.
 */
struct cpu_set_s
{
  /** The number of CPUs in this set. */
  int cpuset_num_cpus;

  /** The CPUs in this set. */
  struct cpu_s* cpuset_cpus;
};


EXIT_NAMESPACE(scheduler);

#endif
