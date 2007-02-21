
/** @file resource_pool.h
 *
 *  @brief A resource pool is used to track the acquisition and use of
 *  a certain resource in the system. The pool's capacity represents
 *  the total number of instances of this resource in the system. The
 *  purpose of this pool is to provide a way for threads to reserve
 *  resources. They will wait until the requested number of resources
 *  becomes available and then proceed. In addition, we provide a way
 *  for threads to record when they actually use these resources. The
 *  caller may use the number of unused (idle) resources to identify
 *  when to create more (and increase the pool capacity).
 *
 *  This implementation uses Pthreads synchronization functions.
 *
 *  @author Naju Mancheril (ngm)
 *
 *  @bug None known.
 */
#ifndef _RESOURCE_POOL_H
#define _RESOURCE_POOL_H



/* exported datatypes */


/** @typedef resource_pool_t
 *
 *  @brief This is our reader-writer lock datatype. Modules that need
 *  access to the internal representation should include
 *  resource_pool_struct.h.
 */
typedef struct resource_pool_s* resource_pool_t;



/* exported functions */

void resource_pool_init(resource_pool_t rp, pthread_mutex_t* mutex, int capacity);
void resource_pool_destroy(resource_pool_t rp);
void resource_pool_reserve(resource_pool_t rp, int n);
void resource_pool_unreserve(resource_pool_t rp, int n);
void resource_pool_notify_capacity_increase(resource_pool_t rp, int diff);
void resource_pool_notify_idle(resource_pool_t rp);
void resource_pool_notify_non_idle(resource_pool_t rp);
int  resource_pool_get_num_capacity(resource_pool_t rp);
int  resource_pool_get_num_reserved(resource_pool_t rp);
int  resource_pool_get_num_non_idle(resource_pool_t rp);



#endif
