
/** @file dispatcher_cpu.cpp
 *
 *  @brief Implements dispatcher_cpu_t functions.
 *
 *  @bug None known.
 */
#include "engine/dispatcher/dispatcher_cpu.h"
#include "engine/dispatcher/dispatcher_cpu_struct.h"

#include "trace.h"
#include "qpipe_panic.h"


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
  {
    TRACE(TRACE_ALWAYS, "sched_setaffinity() failed\n");
    QPIPE_PANIC();
  }
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
  {
    TRACE(TRACE_ALWAYS, "processor_bind() failed\n");
    QPIPE_PANIC();
  }
  return;

#endif
#endif
  


  TRACE(TRACE_ALWAYS, "Unsupported operating system\n");
  QPIPE_PANIC();
}


int  dispatcher_cpu_get_unique_id(dispatcher_cpu_t cpu) {
  return cpu->cpu_unique_id;
}
