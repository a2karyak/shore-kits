// -*- mode:c++; c-basic-offset:4 -*-
#include "util/clh_lock.h"
#include <cassert>

#if defined(__SUNPRO_CC)
#include <atomic.h>
#endif

/** Each lock and each thread owns a qnode. A node becomes active when
    it joins the queue of some clh_lock, and becomes inactive when a
    thread finishes spinning on it. This means that whenever a thread
    uses a lock, it loses ownership of its qnode (because it has no
    way to know when/if its successor) will finish
    spinning. Fortunately, whenever the thread releases the lock, its
    predecessor's node is free and can be recycled.    
*/

clh_lock::qnode_ptr &clh_lock::me() {
    static __thread qnode_ptr _me;
    return _me;
}


void clh_lock::acquire() {
    qnode_ptr i = me();
#ifdef DEBUG
    i->_tid = pthread_self();
#endif
    i->_waiting = true;
#if defined(__SUNPRO_CC)
    qnode_ptr pred = (qnode_ptr) atomic_swap_ptr(&_tail, (void*) i);
    membar_enter();

#elif defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8)
    /* Use a "lock acquire" to update the tail pointer. 

       WARNING: this is a slight abuse of the intrinsic in several ways
       See http://gcc.gnu.org/onlinedocs/gcc/Atomic-Builtins.html and
       http://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html

       1. gcc doesn't actually promise to store the value we gave it (the
       target arch might not have a way to perform atomic swaps. I'm
       assuming that gcc will use CAS to implement this, assuming we have
       CAS (hence the test for __GCC_HAVE_SYNC_COMPARE_AND_SWAP_8). I've
       verified that the following setups work: ia64-linux-gnu-4.1.2
       powerpc64-unknown-linux-gnu-4.1.2, x86_64-unknown-linux-gnu-4.2.0

       2. We haven't (necessarily) acquired the lock yet when this
       operation completes. However, moving an acquire barrier forward
       will still prevent our caller from speculating into the critical
       section this lock protects. 

       3. When we release the lock, we won't be updating the same memory
       location we updated to acquire the lock. This shouldn't matter,
       because memory barriers are not address-specific anyway.
    */
    qnode_ptr pred = __sync_lock_test_and_set(&_tail, i);
#else
#error CLH locks unsuppored on this arch
#endif

    assert(i != pred || !pred->_waiting);

    // now spin
    spin(i->_pred = pred);
}

void clh_lock::release() {
    // we need a reference this time
    qnode_ptr &i = me();
    qnode_ptr old_ptr = i;

    // do our recycling now to avoid races (bug fix)
    i = i->_pred;
    
#if defined(__SUNPRO_CC)
    old_ptr->_waiting = false;
    membar_exit();
#elif defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8)
    // see notes in acquire()
    __sync_lock_release(&old_ptr->_waiting);
#endif
}
