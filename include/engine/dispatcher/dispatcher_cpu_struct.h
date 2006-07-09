/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file dispatcher_cpu_struct.h
 *
 *  @brief Exports dispatcher_cpu_s structure. Do not include this
 *  file unless a module needs to statically create these
 *  structures. All manipulations should be done on a dispatcher_cpu_t
 *  instance using one of the exported macros.
 *
 *  @bug See dispatcher_cpu.cpp.
 */
#ifndef _DISPATCHER_CPU_STRUCT_H
#define _DISPATCHER_CPU_STRUCT_H

#include "engine/dispatcher/dispatcher_os_support.h"



/* operating system specific includes */


/* GNU Linux */
#if defined(linux) || defined(__linux)

/* detected GNU Linux */
#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <sched.h>


#endif


/* Sun Solaris */
#if defined(sun) || defined(__sun)
#if defined(__SVR4) || defined(__svr4__)

/* detected Sun Solaris */
#include <sys/types.h>
#include <sys/processor.h>

#endif
#endif




/* exported structures */

/**
 *  @brief Architecture-specific representation of a CPU.
 */
struct dispatcher_cpu_s
{

  /* regardless of architecture, we would like to provide a unique
     integer id for each CPU so we can print them */
  int cpu_unique_id;


  /* GNU Linux */
#if defined(linux) || defined(__linux)
#ifndef __USE_GNU
#define __USE_GNU
#endif

  /* detected GNU Linux */
  /** One of the operations we would like to provide on a CPU is the
      ability of a thread to bind to it. We implement processor
      binding in Linux using the hard affinity library functions
      provided by glibc. This cpu_set_t should be initialized to
      contain a single CPU (this one). */
  cpu_set_t cpu_set;
 
#endif

  
  /* Sun Solaris */
#if defined(sun) || defined(__sun)
#if defined(__SVR4) || defined(__svr4__)
  
  /* detected Sun Solaris */
  /** One of the operations we would like to provide on a CPU is the
      ability of a thread to bind to it. We implement processor
      binding in Solaris using the processor_bind() system call. This
      field is initialized to this CPU's ID. */
  processorid_t    cpu_id;

  /** We use the processor_info() system call to ensure that a CPU of
      a specified ID is available for binding. Rather than throw the
      returned information away, we store it. */
  processor_info_t cpu_proc_info;

#endif
#endif

};



#endif
