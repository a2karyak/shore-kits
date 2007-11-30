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
    typedef std::pair<LockType*, LiveHandle> live_handle;
    typedef std::vector<live_handle> lh_list;
    lh_list _live_handles;
    std::vector<DeadHandle> _dead_handles;
    
    struct match_lock {
	LockType* _lock;
	match_lock(LockType* l) : _lock(l) { }
	bool operator()(live_handle const &h) { return h.first == _lock; }
    };

    // associates a live handle with a lock
    void put_me(LockType* lock, LiveHandle h) {
	_live_handles.push_back(live_handle(lock, h));
    }
    
    // finds the live handle for this lock and clears its association
    LiveHandle get_me(LockType* lock) {
	typedef typename lh_list::iterator iterator;
	
	iterator it = std::find_if(_live_handles.begin(), _live_handles.end(), match_lock(lock));
	assert(it != _live_handles.end());
	LiveHandle h = it->second;
	*it = _live_handles.back(); // fill the hole with the last entry 
	_live_handles.resize(_live_handles.size()-1);
	return h;	    
    }
    void free(DeadHandle h) {
	_dead_handles.push_back(h);
    }
    DeadHandle alloc() {
	if(_dead_handles.empty())
	    return LockType::create_handle();

	DeadHandle h = _dead_handles.back();
	_dead_handles.pop_back();
	return h;
    }
    
    ~clh_manager() {
	// we better not be holding any locks!
	assert(_live_handles.empty());
	std::for_each(_dead_handles.begin(), _dead_handles.end(), LockType::destroy_handle);
    }
};



#endif
