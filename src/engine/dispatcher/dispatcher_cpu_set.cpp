
/** @file dispatcher_cpu_set.cpp
 *
 *  @brief Implements dispatcher_cpu_set_t functions.
 *
 *  @bug None known.
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "engine/dispatcher/dispatcher_cpu_set.h"
#include "engine/dispatcher/dispatcher_cpu_set_struct.h"

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

static void cpu_set_copy( cpu_set_t* dst, cpu_set_t* src );
static int dispatcher_cpu_set_init_Linux(dispatcher_cpu_set_t cpu_set);

#endif


/* Sun Solaris */
#if defined(sun) || defined(__sun)
#if defined(__SVR4) || defined(__svr4__)

/* detected Sun Solaris */
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>

static int dispatcher_cpu_set_init_Solaris(dispatcher_cpu_set_t cpu_set);

#endif
#endif





/* definitions of exported functions */


/**
 *  @brief Initialize the specified dispatcher_cpu_set_t
 *  instance. Should be called on a CPU set before it is used.
 *
 *  This function uses malloc() to allocate variable sized data.
 *
 *  @param cpu_set The CPU set.
 *
 *  @return 0 on success. Negative value on error.
 */
int dispatcher_cpu_set_init(dispatcher_cpu_set_t cpu_set)
{

  /* GNU Linux */
#if defined(linux) || defined(__linux)
#ifndef __USE_GNU
#define __USE_GNU
#endif

  /* detected GNU Linux */
  return dispatcher_cpu_set_init_Linux( cpu_set );

#endif
  

  
  /* Sun Solaris */
#if defined(sun) || defined(__sun)
#if defined(__SVR4) || defined(__svr4__)
  
  /* detected Sun Solaris Linux */
  return dispatcher_cpu_set_init_Solaris( cpu_set );

#endif
#endif



  TRACE(TRACE_ALWAYS, "Unsupported operating system\n");
  QPIPE_PANIC();
  return -1; /* control never reaches here */
}




/**
 *  @brief Return the number of CPUs in this dispatcher_cpu_set_t
 *  instance.
 *
 *  @param cpu_set The CPU set.
 *
 *  @return The number of CPUs in this dispatcher_cpu_set_t instance.
 */
int dispatcher_cpu_set_get_num_cpus(dispatcher_cpu_set_t cpu_set)
{
  /* error checks */
  if ( cpu_set == NULL )
  {
    TRACE(TRACE_ALWAYS, "Called with NULL dispatcher_cpu_set_t\n");
    QPIPE_PANIC();
  }
  return cpu_set->cpuset_num_cpus;
}




/**
 *  @brief Return the specified CPU from the dispatcher_cpu_set_t
 *  instance.
 *
 *  @param cpu_set The CPU set.
 *
 *  @param index The index of the CPU. Must be between 0 and the
 *  result of dispatcher_cpu_set_get_num_cpus(cpu_set).
 *
 *  @return NULL on error. On success, returns the specified CPU.
 */
dispatcher_cpu_t dispatcher_cpu_set_get_cpu(dispatcher_cpu_set_t cpu_set, int index)
{

  /* error checks */
  if ( cpu_set == NULL )
  {
    TRACE(TRACE_ALWAYS, "Called with NULL dispatcher_cpu_set_t\n");
    QPIPE_PANIC();
  }
  if ( index < 0 )
  {
    TRACE(TRACE_ALWAYS, "Called with negative index %d\n", index);
    QPIPE_PANIC();
  }
  if ( index >= cpu_set->cpuset_num_cpus )
  {
    TRACE(TRACE_ALWAYS,
          "Called with index %d in a dispatcher_cpu_set_t with %d CPUs\n",
          index,
          cpu_set->cpuset_num_cpus);
    QPIPE_PANIC();
  }
  
  return &cpu_set->cpuset_cpus[index];
}




/**
 *  @brief Destroy all resources allocated for this CPU set. Should be
 *  called on a CPU set when it is no longer needed.
 *
 *  @param cpu_set The CPU set.
 *
 *  @return void
 */
void dispatcher_cpu_set_finish(dispatcher_cpu_set_t cpu_set)
{
  cpu_set->cpuset_num_cpus = 0;
  free( cpu_set->cpuset_cpus );
  cpu_set->cpuset_cpus = NULL;
}





/* definition of internal helper functions */


/* GNU Linux */
#if defined(linux) || defined(__linux)
#ifndef __USE_GNU
#define __USE_GNU
#endif


/* detected GNU Linux */

/**
 *  @brief Copy a Linux cpu_set_t.
 *
 *  @param dst Will be the copy.
 *
 *  @param src The source (will be copied).
 *
 *  @return void
 */
static void cpu_set_copy( cpu_set_t* dst, cpu_set_t* src )
{
  int i;

  CPU_ZERO( dst );
  for (i = 0; i < CPU_SETSIZE; i++)
    if ( CPU_ISSET(i, src) )
      CPU_SET(i, dst);
}


/**
 *  @brief Linux-specific version of do_dispatcher_cpu_set_init().
 *
 *  @param cpu_set The CPU set.
 *
 *  @return 0 on success. Negative value on error.
 */
static int dispatcher_cpu_set_init_Linux(dispatcher_cpu_set_t cpu_set)
{

  int i;
  cpu_set_t original_affinity_set;
  int num_cpus = sysconf(_SC_NPROCESSORS_CONF);

  
  
  /* get current affinity set so we can restore it when we're done */
  if ( sched_getaffinity( 0, sizeof(cpu_set_t), &original_affinity_set ) )
  {
    TRACE(TRACE_ALWAYS,
          "sched_getaffinity() failed to get the original affinity set\n");
    return -1;
  }

  /* test restoration */
  if ( sched_setaffinity( 0, sizeof(cpu_set_t), &original_affinity_set ) )
  {
    TRACE(TRACE_ALWAYS,
          "sched_setaffinity() failed to set affinity "
          "to the original affinity set\n");
    return -1;
  }



  /* allocate cpus */
  dispatcher_cpu_t cpus = 
    (dispatcher_cpu_t)malloc( num_cpus * sizeof(struct dispatcher_cpu_s) );
  if ( cpus == NULL )
  {
    TRACE(TRACE_ALWAYS,
          "malloc() failed on dispatcher_cpu_t array of %d elements\n",
          num_cpus);
    return -1;
  }
  for (i = 0; i < num_cpus; i++)
    /* initialize fields */
    CPU_ZERO( &cpus[i].cpu_set );



  /* find the CPUs on the system */
  int num_found = 0;
  int cpu_num;
  for (cpu_num = 0; ; cpu_num++)
  {
    cpu_set_t test_set;
    CPU_ZERO( &test_set );
    CPU_SET ( cpu_num, &test_set );

    if ( !sched_setaffinity( 0, sizeof(cpu_set_t), &test_set ) )
    {
      /* found a new CPU */
      cpus[num_found].cpu_unique_id = cpu_num;
      cpu_set_copy( &cpus[num_found].cpu_set, &test_set );
      num_found++;
      if ( num_found == num_cpus )
        break;
    }
  }  

  

  /* restore original affinity set */
  if ( sched_setaffinity( 0, sizeof(cpu_set_t), &original_affinity_set ) )
  {
    TRACE(TRACE_ALWAYS, "sched_setaffinity() failed to restore affinity set\n");
    free(cpus);
    return -1;
  }
  
  
  /* return parameters */
  cpu_set->cpuset_num_cpus = num_cpus;
  cpu_set->cpuset_cpus     = cpus;
  return 0;
}


#endif



/* Sun Solaris */
#if defined(sun) || defined(__sun)
#if defined(__SVR4) || defined(__svr4__)

/* detected Sun Solaris */

/**
 *  @brief Solaris-specific version of do_dispatcher_cpu_set_init().
 *
 *  @param cpu_set The CPU set.
 *
 *  @return 0 on success. Negative value on error.
 */
static int dispatcher_cpu_set_init_Solaris(dispatcher_cpu_set_t cpu_set)
{

  int i;
  int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);


  /* allocate cpus */
  dispatcher_cpu_t cpus =
    (dispatcher_cpu_t)malloc( num_cpus * sizeof(struct dispatcher_cpu_s) );
  if ( cpus == NULL )
  {
    TRACE(TRACE_ALWAYS,
          "malloc() failed on dispatcher_cpu_t array of %d elements\n",
          num_cpus);
    return -1;
  }
  for (i = 0; i < num_cpus; i++)
  {
    /* initialize fields */
    memset( &cpus[i], 0, sizeof(struct dispatcher_cpu_s) );
  }


  /* find the CPUs on the system */
  int num_found = 0;
  int cpu_num;
  for (cpu_num = 0; ; cpu_num++)
  {
    int status = p_online(cpu_num, P_STATUS);
    if ( (status == -1) && (errno == EINVAL) )
      continue;
    
    /* found a new CPU */
    cpus[num_found].cpu_unique_id = cpu_num;
    cpus[num_found].cpu_id = cpu_num;
    if ( processor_info( cpu_num, &cpus[num_found].cpu_proc_info ) )
    {
      TRACE(TRACE_ALWAYS, "processor_info() failed on CPU %d/%d\n",
            cpu_num+1,
            num_cpus);
      free(cpus);
      return -1;
    }

    num_found++;
    if ( num_found == num_cpus )
      break;
  }


  /* return parameters */
  cpu_set->cpuset_num_cpus = num_cpus;
  cpu_set->cpuset_cpus     = cpus;
  return 0;
}

#endif
#endif
