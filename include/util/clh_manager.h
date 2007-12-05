/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef __CLH_LM_H
#define __CLH_LM_H

#include <list>
#include <utility>
#include <vector>
#include <cassert>
#include <algorithm>

/* a thread-local lock handle manager for locks that depend on the
   user to maintain storage for some of their state (ie CLH). It
   creates, tracks, and destroys lock handles as needed so the
   application doesn't have to.
*/
template<class LockType>
struct clh_manager {
    typedef typename LockType::live_handle LiveHandle;
    typedef typename LockType::dead_handle DeadHandle;
    union handle {
	LiveHandle _lh;
	DeadHandle _dh;
    };
    enum { MAX_HANDLES=4 }; // really 2-3 should be enough!
    handle _handles[MAX_HANDLES];
    LockType const* _locks[MAX_HANDLES];
    

    // a known bad lock pointer that marks available (dead) lock handles
    static LockType const* dead_lock() { return (LockType const*) 0x1; }
    
    clh_manager() {
	for(int i=0; i < MAX_HANDLES; i++) {
	    _handles[i]._dh = LockType::create_handle();
	    _locks[i] = dead_lock();
	}
    };
    ~clh_manager() {
	// we better not be holding any locks!
	for(int i=0; i < MAX_HANDLES; i++) {
	    assert(_locks[i] == dead_lock());
	    LockType::destroy_handle(_handles[i]._dh);
	}
    }

    inline
    int find(LockType const* lock) {
	int i=0;
	while( i < MAX_HANDLES && _locks[i] != lock) i++;
	//	assert(i < MAX_HANDLES);
	return i;
    }
    // associates a live handle with a lock
    #if 1
    inline
    void put_me(LockType const* lock, LiveHandle h) {	
	int i = find(NULL);
	_locks[i] = lock;
	_handles[i]._lh = h;
    }
    
    // finds the live handle for this lock and clears its association
    inline
    LiveHandle get_me(LockType const* lock) {
	int i = find(lock);
	LiveHandle h = _handles[i]._lh;
	_locks[i] = NULL; // free the slot
	return h;
    }
    inline
    void free(DeadHandle h) {
	int i = find(NULL);
	_locks[i] = dead_lock();
	_handles[i]._dh = h;
    }
    inline
    DeadHandle alloc() {
	int i = find(dead_lock());
	_locks[i] = NULL;
	return _handles[i]._dh;
    }
    #else
    // cheat!
    void put_me(LockType const* lock, LiveHandle h) { _handles[0]._lh = h; }
    LiveHandle get_me(LockType const* lock) { return _handles[0]._lh; }
    void free(DeadHandle h) { _handles[0]._dh = h; }
    DeadHandle alloc() { return _handles[0]._dh; }
    #endif
    
};



#endif
