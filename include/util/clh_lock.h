// -*- mode:c++; c-basic-offset:4 -*-
#ifndef __CLHLOCK_H
#define __CLHLOCK_H

#include "util/clh_manager.h"

#if defined(__SUNPRO_CC)
#include <atomic.h>
#endif


/** A CLH queing spinlock. Contention is limited to a single SWAP,
    after which each thread spins on its own memory location.

    In theory, this implementation is as fast as test-and-set locks
    for an uncontended lock. With contention, a thread can expect to
    acquire the lock in roughly the time it takes for all critical
    sections to complete (which tends to be significantly longer than
    the locking overhead, even for short critical sections).
*/
class clh_lock {

    struct qnode;
    typedef qnode volatile* qnode_ptr;
    // A queue entry for a thread to spin on
    struct qnode {
	qnode_ptr _pred;
	bool volatile _waiting;
	qnode() : _pred(NULL), _waiting(false) { }
    };
  
    // 
    static void spin(qnode_ptr pred); // in clh_profile.cpp
    
private:
    qnode_ptr volatile _tail;

public:
    
    // clh_manager uses these
    typedef qnode_ptr live_handle;
    typedef qnode_ptr dead_handle;
    static dead_handle create_handle() { return new qnode; }
    static void destroy_handle(dead_handle h) { delete h; }

    // node manager
    typedef clh_manager<clh_lock> Manager;
    static __thread Manager* _manager; // accesses my thread-local clh_manager
    static void thread_init_manager() { _manager = new Manager; }
    static void thread_destroy_manager() { delete _manager; }
    
    // the lock starts out free
    clh_lock() : _tail(create_handle()) { }
    ~clh_lock() { destroy_handle(_tail); }

    live_handle acquire(dead_handle i) {
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
    dead_handle release(live_handle i) {
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
    void acquire() {
	Manager* m = _manager;
	m->put_me(this, acquire(m->alloc()));
    }
    void release() {
	Manager* m = _manager;
	m->free(release(m->get_me(this)));
    }
    
};
#endif
