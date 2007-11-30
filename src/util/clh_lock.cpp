// -*- mode:c++; c-basic-offset:4 -*-
#include "util/clh_lock.h"
#include <cassert>

/** Each lock and each thread owns a qnode. A node becomes active when
    it joins the queue of some clh_lock, and becomes inactive when a
    thread finishes spinning on it. This means that whenever a thread
    uses a lock, it loses ownership of its qnode (because it has no
    way to know when/if its successor) will finish
    spinning. Fortunately, whenever the thread releases the lock, its
    predecessor's node is free and can be recycled.    
*/
#if 0
clh_lock::Manager* &clh_lock::manager() {
    static __thread Manager* _manager;
    return _manager;
}


clh_lock::live_handle clh_lock::acquire(clh_lock::dead_handle i) {
    i->_waiting = true;
    
#if defined(__SUNPRO_CC)
    membar_producer(); // make sure _waiting sticks
    qnode_ptr pred = (qnode_ptr) atomic_swap_ptr(&_tail, (void*) i);
#else
    // this forms an "acquire" barrier
    qnode_ptr pred = __sync_lock_test_and_set(&_tail, i);
#endif

    // now spin
    spin(i->_pred = pred);
#ifdef __SUNPRO_CC
    //    membar_enter(); // no later load/store can move in front of this
#endif
    return i;
}

clh_lock::dead_handle clh_lock::release(clh_lock::live_handle i) {
    qnode_ptr pred = i->_pred;

#if defined(__SUNPRO_CC)
    membar_exit();
    i->_waiting = false;
#else
    // see notes in acquire()
    __sync_lock_release(&i->_waiting);
#endif
    return pred;
}
#endif
