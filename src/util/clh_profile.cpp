// -*- mode:c++; c-basic-offset:4 -*-
/** This file exists only to prevent the compiler from inlining
    certain functions that are particularly useful for profiling (like
    spinning). If CC had something similar to gcc's
    __attribut__((noinline)) we wouldn't need to do this...
 */
#include "util/clh_lock.h"
#include "util/clh_rwlock.h"

// FIXME: support gcc!
#ifdef __SUNPRO_CC
#include <atomic.h>
#endif

void clh_lock::spin(clh_lock::qnode_ptr pred) {
    while(pred->_waiting);
}

#ifdef TRACE_MODE
#ifndef SERIAL_MODE
#define SERIAL_MODE
#endif
#endif

#ifdef SERIAL_MODE
#ifndef DEBUG_MODE
#define DEBUG_MODE
#endif
#endif

/*
  Automatic thread-local lock handle management

  It would be *really* nice if the compiler could handle arbitrary
  initializers for TLS, but we can't use operator new(). There is
  already a proposal (n2147) to make TLS part of C++, with full
  support for dynamic initialization, but who knows if/when that will
  happen -- the elegant solution would require OS loader, linker and
  compiler support, so we have a bit of chicken-and-egg here.

  NOTE: even though this is imminently inlineable, it must appear
  exactly once per executable, so it's here instead of the header
  file.
*/
clh_rwlock::Manager* &clh_rwlock::manager() {
    static __thread Manager* m = NULL;
    return m;
}

void clh_rwlock::acquire_read() {
    Manager* m = manager();
    m->put_me(this, acquire_read(m->alloc()));
}

void clh_rwlock::acquire_write() {
    Manager* m = manager();
    m->put_me(this, acquire_write(m->alloc()));
}

void clh_rwlock::release() {
    Manager* m = manager();
    m->free(release(m->get_me(this)));
}


clh_rwlock::dnode clh_rwlock::lnode::spin() {
#ifdef TRACE_MODE
    fprintf(stderr, "%s* (%ld) %d? *\n", get_indent(), get_id(_ptr), _ptr->_tid);
//    fprintf(stderr, "(%ld):\t\t\tt@%d -> t@%d\n", get_id(_ptr), _ptr->_tid, pthread_self());
#endif
    // wait for the previous node to update the count
    while(is_waiting()) {
#ifdef SERIAL_MODE
	for(int i=0; i < 1000 && is_waiting(); i++) {
	    if(is_waiting()) {
		pthread_mutex_unlock(&serialize);
		sched_yield();
		pthread_mutex_lock(&serialize);
	    }
	}
#endif
    }
    membar_consumer();
#ifdef TRACE_MODE
    // by passing the baton, we remove ourselves from the queue
    fprintf(stderr, "%s! (%ld) %d ! \n", get_indent(), get_id(_ptr), _ptr->_tid);
//    fprintf(stderr, "(%ld):\t\t\t\t\t !  -> t@%d\n", get_id(_ptr), pthread_self());
    _ptr->_pred = NULL;
#endif
    // FIXME: add support for GCC
    membar_enter();
    return kill();
}
