
/** @file resource_pool_struct.h
 *
 *  @brief Exports the internal representation of a resource_pool_t
 *  datatype. This file should be included in a module for the sole
 *  purpose of statically allocating resource_pool_s structures (i.e. as
 *  globals).
 *
 *  Any module that needs the resource_pool_t definition and prototypes
 *  should include resource_pool.h directly.
 *
 *  Modules should only include this file IF AND ONLY IF they need to
 *  statically allocate and/or initialize resource_pool_s structures. Once
 *  that is done, this module should continue to use the functions
 *  provided in resource_pool.h to manipulate the data structure.
 *
 *  @author Naju Mancheril (ngm)
 *
 *  @bug See resource_pool.c.
 */
#ifndef _RESOURCE_POOL_STRUCT_H
#define _RESOURCE_POOL_STRUCT_H

#include "util/static_list_struct.h" /* for static_list_s structure */



/* exported structures */


/** @struct resource_pool_s
 *
 *  @brief This structure represents our reader writer lock. We expose
 *  it here so that these structures may be statically allocated. Once
 *  an instance is allocated, it should be passed around as a pointer
 *  (a resource_pool_t).
 */
struct resource_pool_s
{
  pthread_mutex_t* mutexp;

  int capacity;
  int reserved;
  int non_idle;

  struct static_list_s waiters;
};



/* exported initializers */


/** @def RESOURCE_POOL_INITIALIZER
 *
 *  @brief Static initializer for a resource_pool_s structure. Can be used to
 *  initialize global and static variables.
 *
 *  @param this Address of the structure we are trying to initialize.
 *
 *  @param mutexp Pointer to the mutex that will protect this
 *  semaphore.
 *
 *  @param capacity Initial value of the semaphore.
 *
 *  This initializer should be used to initialize a variable 'lock' in
 *  the following way:
 *
 *  struct resource_pool_s lock = RESOURCE_POOL_STATIC_INITIALIZER(&lock, mutexp, 10);
 */
#define RESOURCE_POOL_STATIC_INITIALIZER(this,mutexp,capacity) { \
    (mutexp), \
    (capacity), \
    0, \
    0, \
    STATIC_LIST_INITIALIZER(&(this)->waiters) \
}



#endif
