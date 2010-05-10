/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
      Data Intensive Applications and Systems Labaratory (DIAS)
               Ecole Polytechnique Federale de Lausanne
   
                         All Rights Reserved.
   
   Permission to use, copy, modify and distribute this software and
   its documentation is hereby granted, provided that both the
   copyright notice and this permission notice appear in all copies of
   the software, derivative works or modified versions, and any
   portions thereof, and that both notices appear in supporting
   documentation.
   
   This code is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
   DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
   RESULTING FROM THE USE OF THIS SOFTWARE.
*/

/** @file cpu_set.cpp
 *
 *  @brief Implements cpu_set_t functions.
 */

#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <cerrno>

#include "util.h"
#include "qpipe/scheduler/cpu_set.h"
#include "qpipe/scheduler/cpu_set_struct.h"
#include "qpipe/scheduler/os_support.h"

ENTER_NAMESPACE(qpipe);

static void cpu_set_init_Solaris(cpu_set_p cpu_set);


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
  /* detected Sun Solaris Linux */
  cpu_set_init_Solaris( cpu_set );
  return;
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
        THROW1(QPipeException, "Called with NULL cpu_set_t");

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
        THROW1(QPipeException, "Called with NULL cpu_set_t");
    
    if ( index < 0 )
        THROW2(OutOfRange, "Called with negative index %d\n", index);
    if ( index >= cpu_set->cpuset_num_cpus )
        THROW3(OutOfRange, 
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
      THROW(BadAlloc);
      
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
        THROW4(ThreadException,
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

EXIT_NAMESPACE(qpipe);

