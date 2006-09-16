
/** @file dispatcher_cpu.cpp
 *
 *  @brief Implements dispatcher_cpu_t functions.
 *
 *  @bug None known.
 */
#include "util.h"
#include "core/dispatcher_cpu.h"

#ifdef  DISPATCHER_UNSUPPORTED_OS
#define DISPATCHER_UNSUPPORTED_OS 1
#endif



/* GNU Linux */
#if defined(linux) || defined(__linux)

/* detected GNU Linux */
#undef  DISPATCHER_UNSUPPORTED_OS
#define DISPATCHER_UNSUPPORTED_OS 0

#endif


/* Sun Solaris */
#if defined(sun) || defined(__sun)
#if defined(__SVR4) || defined(__svr4__)

/* detected Sun Solaris */
#undef  DISPATCHER_UNSUPPORTED_OS
#define DISPATCHER_UNSUPPORTED_OS 0

#endif
#endif



#if DISPATCHER_UNSUPPORTED_OS
#error "Unsupported operating system\n"
#endif

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

/* operating system specific */

/* GNU Linux */
#if defined(linux) || defined(__linux)
#ifndef __USE_GNU
#define __USE_GNU
#endif

/* detected GNU Linux */
#include <sched.h>

#endif


/* Sun Solaris */
#if defined(sun) || defined(__sun)
#if defined(__SVR4) || defined(__svr4__)

/* detected Sun Solaris */
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>

#endif
#endif





/* definitions of exported functions */


/**
 *  @brief Bind the calling thread to the specified CPU. We cannot run
 *  QPipe if any of the system calls that this function relies on
 *  fails, so we deal with errors by invoking QPIPE_PANIC().
 *
 *  @param cpu_info The cpu. Should be initialized by
 *  dispatcher_init_info().
 *
 *  @return void
 */
void dispatcher_cpu_bind_self(dispatcher_cpu_t cpu)
{


  /* GNU Linux */
#if defined(linux) || defined(__linux)

  /* detected GNU Linux */
  /* sched_setaffinity() sets the CPU affinity mask of the process
     denoted by pid.  If pid is zero, then the current process is
     used. We really want to bind the current THREAD, but in Linux, a
     THREAD is a processor with its own pid_t. */
  
  if ( sched_setaffinity(0, sizeof(cpu_set_t), &cpu->cpu_set) )
      throw EXCEPTION(QPipeException,
                      "Caught %s in call to sched_setaffinity()",
                      errno_to_str().data());

  return;

#endif
  
  
  
  /* Sun Solaris */
#if defined(sun) || defined(__sun)
#if defined(__SVR4) || defined(__svr4__)
  
  /* detected Sun Solaris */
  /* The processor_bind() function binds the LWP (lightweight process)
     or set of LWPs specified by idtype and id to the processor
     specified by processorid. If obind is not NULL, this function
     also sets the processorid_t variable pointed to by obind to the
     previous binding of one of the specified LWPs, or to PBIND_NONE
     if the selected LWP was not bound.

     If id is P_MYID, the specified LWP, process, or task is the
     current one. */

  if ( processor_bind(P_LWPID, P_MYID, cpu->cpu_id, NULL) )
      throw EXCEPTION(QPipeException,
                      "Caught %s while binding processor",
                      errno_to_str().data());
  return;

#endif
#endif
  

  throw EXCEPTION(QPipeException, 
                  "Unsupported operating system\n");
}


int  dispatcher_cpu_get_unique_id(dispatcher_cpu_t cpu) {
  return cpu->cpu_unique_id;
}
