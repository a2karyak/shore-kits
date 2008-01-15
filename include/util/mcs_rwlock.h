/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file mcs_rwlock.h
 *
 *  @brief Implementation of MCS-based rwlock
 */

#ifndef __MCS_RW_LOCK_H
#define __MCS_RW_LOCK_H


#include "util/atomic_ops.h"


struct mcs_rwlock {
    struct qnode;
    typedef qnode volatile* qnode_ptr;
    struct qnode {
	qnode_ptr _prev;
	qnode_ptr _next;
	enum State {READER, WRITER, ACTIVE_READER};
	State _state;
	unsigned int _waiting;
	unsigned int _locked;
	qnode() : _prev(NULL), _next(NULL), _waiting(true), _locked(false) { }
	void lock() volatile {
#ifdef __SUNPRO_CC
	    while(_locked || atomic_cas_32(&_locked, false, true));
	    membar_enter();
#else
	    while(__sync_lock_test_and_set(&_locked, true));
#endif
	}
	void unlock() volatile {
#ifdef __SUNPRO_CC
	    membar_exit();
	    _locked = false;
#else
	    __sync_lock_release(&_locked);
#endif
	}
    };

    qnode_ptr volatile _tail;

    mcs_rwlock() : _tail(NULL) { }

    qnode_ptr join_queue(qnode_ptr me) {
#ifdef __SUNPRO_CC
	membar_producer();
	qnode_ptr pred = atomic_swap(&_tail, me);
#else
	__sync_synchronize();
	qnode_ptr pred = __sync_lock_test_and_set(&_tail, me);
#endif
	return pred;
    }
    qnode_ptr try_fast_leave_queue(qnode_ptr me, qnode_ptr new_value=NULL) {
	qnode_ptr next;
#ifdef __SUNPRO_CC
	membar_exit();
	if(!(next=me->_next) && me == atomic_cas(&_tail, me, new_value))
	    return NULL;
#else
	if(!(next=me->_next) && __sync_bool_compare_and_swap(&_tail, me, new_value))
	    return NULL;
#endif
	while(!(next=me->_next));
	return next;
    }
    void acquire_write(qnode_ptr me) {
	me->_state = qnode::WRITER;
	me->_waiting = true;
	me->_next = NULL;
	qnode_ptr pred = join_queue(me);
	if(pred) {
	    pred->_next = me;
	    while(me->_waiting);
#ifndef __SUNPRO_CC
	    __sync_synchronize(); 
#endif
	}
#ifdef __SUNPRO_CC
	membar_enter();
#endif
    }

    void release_write(qnode_ptr me) {
	qnode_ptr next = try_fast_leave_queue(me);
	if(!next) 
	    return;
	
	next->_prev = NULL;
#ifdef __SUNPRO_CC
	membar_producer();
	next->_waiting = false;
#else
	__sync_lock_release(&next->_waiting);
#endif
    }

    void acquire_read(qnode_ptr me) {
	me->_state = qnode::READER;
	me->_waiting = true;
	me->_next = me->_prev = NULL;
	qnode_ptr pred = join_queue(me);
	if(pred) {
	    me->_prev = pred;
	    qnode::State pred_state = pred->_state;
#ifdef __sparcv9
		membar_consumer();
#else
		__sync_synchronize();
#endif
	    pred->_next = me;
	    if(pred_state != qnode::ACTIVE_READER) {
		while(me->_waiting);
#ifdef __sparcv9
		membar_consumer();
#else
		__sync_synchronize();
#endif
	    }
	}
	if(me->_next && me->_next->_state == qnode::READER) 
	    me->_next->_waiting = false;
	
	me->_state = qnode::ACTIVE_READER;
#ifdef __sparcv9
	membar_enter();
#else
	__sync_synchronize();
#endif
    }

    void release_read(qnode_ptr me) {
#ifdef __sparcv9
	membar_exit();
#else
	__sync_synchronize();
#endif
	qnode_ptr pred = me->_prev;

	// try to lock down my predecessor
	while(pred) {
	    pred->lock();
	    if(me->_prev == pred)
		break;
	    
	    pred->unlock();
	    pred = me->_prev;
	}
	
	me->lock();
	if(pred) {
	    // make my predecessor the tail of the queue
	    pred->_next = NULL;
	    qnode_ptr next = try_fast_leave_queue(me, pred);
	    if(next) {
		next->_prev = pred;
		pred->_next = next;
	    }
	    pred->unlock();
	}
	else {
	    // make my successor the head of the queue
	    qnode_ptr next = try_fast_leave_queue(me);
	    if(next) {
#ifdef __sparcv9
		membar_producer();
		next->_waiting = false;
#else
		__sync_lock_release(&next->_waiting);
#endif
		next->_prev = NULL; // unlink myself
	    }
	}
	me->unlock();
    }
    
}; // EOF: mcs_rwlock 


#endif
