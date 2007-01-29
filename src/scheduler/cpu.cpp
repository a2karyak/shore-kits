
/** @file cpu.cpp
 *
 *  @brief Implements cpu_t functions.
 *
 *  @bug None known.
 */
#include "util.h"
#include "scheduler/cpu.h"
#include "scheduler/cpu_struct.h"
#include "scheduler/os_support.h"


ENTER_NAMESPACE(scheduler);

/* definitions of exported functions */


/**
 *  @brief Bind the calling thread to the specified CPU. We cannot run
 *  QPipe if any of the system calls that this function relies on
 *  fails, so we deal with errors by invoking QPIPE_PANIC().
 *
 *  @param cpu_info The cpu. Should be initialized by
 *  init_info().
 *
 *  @return void
 */
void cpu_bind_self(cpu_t cpu)
{


  /* GNU Linux */
#ifdef FOUND_LINUX

  /* detected GNU Linux */
  /* sched_setaffinity() sets the CPU affinity mask of the process
     denoted by pid.  If pid is zero, then the current process is
     used. We really want to bind the current THREAD, but in Linux, a
     THREAD is a processor with its own pid_t. */
  
  if ( sched_setaffinity(0, sizeof(os_cpu_set_t), &cpu->cpu_set) )
    throw EXCEPTION2(QPipeException,
                     "Caught %s in call to sched_setaffinity()",
                     errno_to_str().data());

  return;

#endif
  
  
  
  /* Sun Solaris */
#ifdef FOUND_SOLARIS
  
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
      THROW2(QPipeException,
                      "Caught %s while binding processor",
                      errno_to_str().data());
  return;

#endif
  

  THROW1(QPipeException, 
                  "Unsupported operating system\n");
}


int  cpu_get_unique_id(cpu_t cpu) {
  return cpu->cpu_unique_id;
}

EXIT_NAMESPACE(scheduler);
