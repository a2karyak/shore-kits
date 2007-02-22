
/** @file resource_pool.cpp
 *
 *  @brief Implements methods described in resource_pool.h.
 *
 *  @author Naju Mancheril (ngm)
 *
 *  Since a thread is not doing anything while waiting to reserve
 *  resources, we can stack allocate the nodes we use to maintain a
 *  list of waiting threads.
 *
 *  If a thread tries to reserve resources and does not have to wait,
 *  it is responsible for updating the state of the resource_pool_s
 *  structure. If the thread waits, then it can assume that the pool's
 *  state has been properly updated when it wakes.
 *
 *  @bug None known.
 */
#include <pthread.h>              /* need to include this first */
#include "util/resource_pool.h"   /* for prototypes */
#include "util/resource_pool_struct.h" /* for resource_pool_s structure definition */
#include "util/static_list.h"          /* for static_list_t functions */

#include <stdlib.h>        /* for NULL */
#include <assert.h>        /* for assert() */
#include "util/trace.h"




/* internal structures */

/**
 * @brief Each waiting thread stack-allocates an instance of this
 * structure. It contains the linked list node structure used to add
 * this thread to the waiter queue.
 */
struct waiter_node_s
{
  int req_reserve_count;
  pthread_cond_t request_granted;
  struct static_list_node_s list_node;
};



/* internal helper functions */

static void wait_for_turn(resource_pool_t rp, int req_reserve_count);
void waiter_wake(resource_pool_t rp);



/* definitions of exported functions */

/** 
 *  @brief Semaphore initializer.
 *
 *  @return void
 */
void resource_pool_init(resource_pool_t rp, pthread_mutex_t* mutexp,
                        int capacity)
{
  /* For some reason, RESOURCE_POOL_STATIC_INITIALIZER does not work. */
  rp->mutexp = mutexp;
  rp->capacity = capacity;
  rp->reserved = 0;
  rp->non_idle = 0;
  static_list_init(&rp->waiters);
}



/**
 *  @brief Reserve 'n' copies of the resource.
 *
 *  @param rp The resource pool.
 *
 *  @param n Wait for this many resources to appear unreserved. If n
 *  is larger than the pool capacity, the behavior is undefined.
 *
 *  THE CALLER MUST BE HOLDING THE INTERNAL MUTEX ('mutexp' used to
 *  initialize 'rp') WHEN CALLING THIS FUNCTION. THE CALLER WILL HOLD
 *  THIS MUTEX WHEN THE FUNCTION RETURNS.
 *
 *  @return void
 */
void resource_pool_reserve(resource_pool_t rp, int n)
{
  
  /* Error checking. If 'n' is larger than the pool capacity, we will
     never be able to satisfy the request. */
  assert(n <= rp->capacity);

  /* Checks:
     
     - If there are other threads waiting, add ourselves to the queue
       of waiters so we can maintain FIFO ordering.

     - If there are no waiting threads, but the number of unreserved
       threads is too small, add ourselves to the queue of waiters. */
  int num_unreserved = rp->capacity - rp->reserved;
  if (!static_list_is_empty(&rp->waiters) || (num_unreserved < n))
  {
    wait_for_turn(rp, n);
    
    /* If we are here, we have been granted the resources. The thread
       which gave them to us has already updated the pool's state. */
    return;
  }


  /* If we are here, we did not wait. We are responsible for updating
     the state of the rpaphore before we exit. */
  rp->reserved += n;
}



/** 
 *  @brief Unreserve the specified number of resources.
 *
 *  @param rp The resource pool.
 *
 *  THE CALLER MUST BE HOLDING THE INTERNAL MUTEX ('mutexp' used to
 *  initialize 'rp') WHEN CALLING THIS FUNCTION. THE CALLER WILL HOLD
 *  THIS MUTEX WHEN THE FUNCTION RETURNS.
 *
 *  @return void
 */
void resource_pool_unreserve(resource_pool_t rp, int n)
{
  /* error checking */
  assert(rp->reserved >= n);

  /* update the 'reserved' count */
  rp->reserved -= n;
  waiter_wake(rp);
  return;
}



/** 
 *  @brief Increase the capacity, releasing any threads whose requests
 *  can now be satisfied.
 *
 *  @param rp The resource pool.
 *
 *  @param diff The increase in capacity.
 *
 *  THE CALLER MUST BE HOLDING THE INTERNAL MUTEX ('mutexp' used to
 *  initialize 'rp') WHEN CALLING THIS FUNCTION. THE CALLER WILL HOLD
 *  THIS MUTEX WHEN THE FUNCTION RETURNS.
 *
 *  @return void
 */
void resource_pool_notify_capacity_increase(resource_pool_t rp, int diff)
{
  assert(diff > 0);
  rp->capacity += diff;
  waiter_wake(rp);
}



/** 
 *  @brief Increase the capacity, releasing any threads whose requests
 *  can now be satisfied.
 *
 *  @param rp The resource pool.
 *
 *  @param diff The increase in capacity.
 *
 *  THE CALLER MUST BE HOLDING THE INTERNAL MUTEX ('mutexp' used to
 *  initialize 'rp') WHEN CALLING THIS FUNCTION. THE CALLER WILL HOLD
 *  THIS MUTEX WHEN THE FUNCTION RETURNS.
 *
 *  @return void
 */
void resource_pool_notify_idle(resource_pool_t rp)
{
  assert(rp->non_idle > 0);
  rp->non_idle--;
}


void resource_pool_notify_non_idle(resource_pool_t rp)
{
  assert(rp->non_idle < rp->reserved);
  rp->non_idle++;
}


int resource_pool_get_num_capacity(resource_pool_t rp)
{
  return rp->capacity;
}


int resource_pool_get_num_reserved(resource_pool_t rp)
{
  return rp->reserved;
}


/**
 * @brief The set of idle resources can be divided into idle and
 * non-idle resources. This method returns the number of idle
 * resources.
 */
int resource_pool_get_num_non_idle(resource_pool_t rp)
{
  return rp->non_idle;
}




/* definitions of internal helper functions */

/**
 * @brief Wait fot the requested number of resources to appear.
 *
 * @param rp The rpaphore.
 *
 * @param req_reserve_count The number of resources the caller wishes
 * to acquire.
 *
 * @pre Calling thread holds resource pool mutex on entry.
 */
static void wait_for_turn(resource_pool_t rp, int req_reserve_count)
{
 
  struct waiter_node_s node;
  node.req_reserve_count = req_reserve_count;
  pthread_cond_init(&node.request_granted, NULL);
  static_list_append(&rp->waiters, &node, &node.list_node);
  
  pthread_cond_wait(&node.request_granted, rp->mutexp);
  
  /* If we are here, we have been granted access! The state of the
     lock has been updated. We just need to return to the caller. */
}



/**
 * @brief Wake as many threads as we can using the new 'reserved'
 * count.
 *
 * @pre Calling thread holds resource pool mutex on entry.
 */
void waiter_wake(resource_pool_t rp)
{
  while ( !static_list_is_empty(&rp->waiters) )
  {
    int num_unreserved = rp->capacity - rp->reserved;

    void* waiter_node;
    int get_ret =
      static_list_get_head(&rp->waiters, &waiter_node);
    assert(get_ret == 0);
    struct waiter_node_s* wn = (struct waiter_node_s*)waiter_node;
    
    if (num_unreserved < wn->req_reserve_count)
      /* Not enough resources to let this thread through... */
      return;
    
    /* Hit another thread that can be allowed to pass. */
    /* Remove thread from queue. Wake it. Update rpaphore count. */
    int remove_ret =
      static_list_remove_head(&rp->waiters, &waiter_node, NULL);
    assert(remove_ret == 0);
    wn = (struct waiter_node_s*)waiter_node;
 
    rp->reserved += wn->req_reserve_count;
    pthread_cond_signal(&wn->request_granted);
  }
}
