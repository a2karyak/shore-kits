// -*- mode:c++; c-basic-offset:4 -*-
#ifndef __CLHLOCK_H
#define __CLHLOCK_H

#include <pthread.h>

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
    typedef qnode_ptr volatile queue;
    // A queue entry for a thread to spin on
    struct qnode {
	queue _pred;
	pthread_t _tid;
	bool volatile _waiting;
	qnode() : _pred(NULL), _tid(0), _waiting(false) { }
    };
  
    // accesses the current thread's qnode
    static qnode_ptr &me();
    static void spin(qnode_ptr pred); // in clh_profile.cpp
private:
    queue _tail;

public:
    /** IMPORTANT: each thread needs its own heap-allocated qnode, and
	thread-local storage can't use operator new(). So, any thread
	that might use a clh_lock during its lifetime MUST call
	thread_init() to avoid seg faulting and thread_destroy() to
	avoid leaking memory. 
     */
    static void thread_init() { me() = new qnode; }
    static void thread_destroy() { delete me(); }
    
    // the lock starts out free
    clh_lock() : _tail(new qnode) { }
    ~clh_lock() { delete _tail; }

    void acquire();
    void release();
};
#endif
