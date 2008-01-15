/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file mcs_lock.h
 *
 *  @brief Implementation of MCS lock
 */

#ifndef __MCS_LOCK_H
#define __MCS_LOCK_H


#include "util/atomic_ops.h"


struct mcs_lock {
    struct qnode;
    typedef qnode volatile* qnode_ptr;
    struct qnode {
	qnode_ptr _next;
	bool _waiting;
	//	qnode() : _next(NULL), _waiting(false) { }
    };
    qnode_ptr volatile _tail;
    mcs_lock() : _tail(NULL) { }
    
    void acquire(qnode volatile &me) {
	me._next = NULL;
	me._waiting = true;
#ifdef __sparcv9
	membar_producer();
	qnode_ptr pred = atomic_swap(&_tail, &me);
#else
	__sync_synchronize();
	qnode_ptr pred = __sync_lock_test_and_set(&_tail, &me);
#endif
	if(pred) {
	    pred->_next = &me;
	    while(me._waiting);
#ifndef __sparcv9
	    __sync_synchronize();
#endif
	}
#ifdef __sparcv9
	membar_enter();
#endif	
    }	
    void release(qnode volatile &me) {
#ifdef __sparcv9
	membar_exit();
#endif
	
	qnode_ptr next;
	if(!(next=me._next)) {
	    qnode_ptr tail = _tail;
	    qnode_ptr new_tail = NULL;
	    if(tail == &me && atomic_cas(&_tail, &me, new_tail) == &me) 
		return;
	    while(!(next=me._next));
	}
#ifdef __sparcv9
	next->_waiting = false;
#else
	__sync_lock_release(&next->_waiting);
#endif
    }

}; // EOF: mcs_lock


#endif
