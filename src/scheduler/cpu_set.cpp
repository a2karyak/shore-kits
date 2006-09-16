
/** @file cpu_set.cpp
 *
 *  @brief Implements cpu_set_t functions.
 *
 *  @bug None known.
 */
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <cerrno>

#include "util.h"
#include "scheduler/cpu_set.h"
#include "scheduler/cpu_set_struct.h"




/* operating system specific */

/* GNU Linux */
#if defined(linux) || defined(__linux)
#ifndef __USE_GNU
#define __USE_GNU
#endif

/* detected GNU Linux */
#include <sched.h>


ENTER_NAMESPACE(scheduler);


static void cpu_set_copy( os_cpu_set_t* dst, os_cpu_set_t* src );
static void cpu_set_init_Linux(cpu_set_p cpu_set);

#endif


/* Sun Solaris */
#if defined(sun) || defined(__sun)
#if defined(__SVR4) || defined(__svr4__)

/* detected Sun Solaris */
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>

ENTER_NAMESPACE(scheduler);

static void cpu_set_init_Solaris(cpu_set_p cpu_set);

#endif
#endif





/* definitions of exported functions */


/**
 *  @brief Initialize the specified cpu_set_t
 *  instance. Should be called on a CPU set before it is used.
 *
 *  This function uses malloc() to allocate variable sized data.
 *
 *  @param cpu_set The CPU set.
 *
 *  @return 0 on success. Negative value on error.
 */
void cpu_set_init(cpu_set_p cpu_set)
{

  /* GNU Linux */
#if defined(linux) || defined(__linux)
#ifndef __USE_GNU
#define __USE_GNU
#endif

  /* detected GNU Linux */
  cpu_set_init_Linux( cpu_set );

#endif
  

  
  /* Sun Solaris */
#if defined(sun) || defined(__sun)
#if defined(__SVR4) || defined(__svr4__)
  
  /* detected Sun Solaris Linux */
  cpu_set_init_Solaris( cpu_set );

#endif
#endif


  throw EXCEPTION(QPipeException, "Unsupported operating system");
}




/**
 *  @brief Return the number of CPUs in this cpu_set_t
 *  instance.
 *
 *  @param cpu_set The CPU set.
 *
 *  @return The number of CPUs in this cpu_set_t instance.
 */
int cpu_set_get_num_cpus(cpu_set_p cpu_set)
{
  /* error checks */
    if ( cpu_set == NULL )
        throw EXCEPTION(QPipeException, "Called with NULL cpu_set_t");

    return cpu_set->cpuset_num_cpus;
}




/**
 *  @brief Return the specified CPU from the cpu_set_t
 *  instance.
 *
 *  @param cpu_set The CPU set.
 *
 *  @param index The index of the CPU. Must be between 0 and the
 *  result of cpu_set_get_num_cpus(cpu_set).
 *
 *  @return NULL on error. On success, returns the specified CPU.
 */
cpu_t cpu_set_get_cpu(cpu_set_p cpu_set, int index)
{

  /* error checks */
    if ( cpu_set == NULL )
        throw EXCEPTION(QPipeException, "Called with NULL cpu_set_t");
    
    if ( index < 0 )
        throw EXCEPTION(OutOfRange, "Called with negative index %d\n", index);
    if ( index >= cpu_set->cpuset_num_cpus )
        throw EXCEPTION(OutOfRange, 
                        "Called with index %d in a cpu_set_t with %d CPUs\n",
                        index,
                        cpu_set->cpuset_num_cpus);
  
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
void cpu_set_finish(cpu_set_p cpu_set)
{
  cpu_set->cpuset_num_cpus = 0;
  free( cpu_set->cpuset_cpus );
  cpu_set->cpuset_cpus = NULL;
}





/* definition of internal helper functions */


/* GNU Linux */
#ifdef FOUND_LINUX

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
static void cpu_set_copy( os_cpu_set_t* dst, os_cpu_set_t* src )
{
  int i;

  CPU_ZERO( dst );
  for (i = 0; i < CPU_SETSIZE; i++)
    if ( CPU_ISSET(i, src) )
      CPU_SET(i, dst);
}


/**
 *  @brief Linux-specific version of do_cpu_set_init().
 *
 *  @param cpu_set The CPU set.
 *
 *  @return 0 on success. Negative value on error.
 */
static void cpu_set_init_Linux(cpu_set_p cpu_set)
{

  int i;
  os_cpu_set_t original_affinity_set;
  int num_cpus = sysconf(_SC_NPROCESSORS_CONF);

  
  
  /* get current affinity set so we can restore it when we're done */
  if ( sched_getaffinity( 0, sizeof(os_cpu_set_t), &original_affinity_set ) )
      throw EXCEPTION(ThreadException,
                      "sched_getaffinity() failed with %s",
                      errno_to_str().data());

  /* test restoration */
  if ( sched_setaffinity( 0, sizeof(os_cpu_set_t), &original_affinity_set ) )
      throw EXCEPTION(ThreadException,
                      "sched_setaffinity() failed with %s",
                      errno_to_str().data());


  /* allocate cpus */
  cpu_t cpus = 
    (cpu_t)malloc( num_cpus * sizeof(struct cpu_s) );
  if ( cpus == NULL )
      throw EXCEPTION(BadAlloc);

  for (i = 0; i < num_cpus; i++)
    /* initialize fields */
    CPU_ZERO( &cpus[i].cpu_set );



  /* find the CPUs on the system */
  int num_found = 0;
  int cpu_num;
  for (cpu_num = 0; ; cpu_num++)
  {
    os_cpu_set_t test_set;
    CPU_ZERO( &test_set );
    CPU_SET ( cpu_num, &test_set );

    if ( !sched_setaffinity( 0, sizeof(os_cpu_set_t), &test_set ) )
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
  if ( sched_setaffinity( 0, sizeof(os_cpu_set_t), &original_affinity_set ) )
      throw EXCEPTION(ThreadException,
                      "sched_setaffinity() failed with %s",
                      errno_to_str().data());
  
  
  /* return parameters */
  cpu_set->cpuset_num_cpus = num_cpus;
  cpu_set->cpuset_cpus     = cpus;
}


#endif



/* Sun Solaris */
#ifdef FOUND_SOLARIS
/* detected Sun Solaris */

/**
 *  @brief Solaris-specific version of do_cpu_set_init().
 *
 *  @param cpu_set The CPU set.
 *
 *  @return 0 on success. Negative value on error.
 */
static void cpu_set_init_Solaris(cpu_set_p cpu_set)
{

  int i;
  int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);


  /* allocate cpus */
  cpu_t cpus =
    (cpu_t)malloc( num_cpus * sizeof(struct cpu_s) );
  if ( cpus == NULL )
      throw EXCEPTION(BadAlloc);
      
  for (i = 0; i < num_cpus; i++)
  {
    /* initialize fields */
    memset( &cpus[i], 0, sizeof(struct cpu_s) );
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
    if ( processor_info( cpu_num, &cpus[num_found].cpu_proc_info ) ) {
        free(cpus);
        throw EXCEPTION(ThreadException,
                        "processor_info() failed with %s on CPU %d/%d\n",
                        errno_to_str().data(),
                        cpu_num+1,
                        num_cpus);
    }

    num_found++;
    if ( num_found == num_cpus )
      break;
  }


  /* return parameters */
  cpu_set->cpuset_num_cpus = num_cpus;
  cpu_set->cpuset_cpus     = cpus;
}

#endif

EXIT_NAMESPACE(scheduler);

