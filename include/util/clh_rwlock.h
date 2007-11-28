/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef __CLH_RWLOCK_H
#define __CLH_RWLOCK_H

#include "util/clh_manager.h"

class clh_rwlock {
private:
    typedef clh_manager<clh_rwlock> Manager;
    
public:
    struct qnode;
    typedef qnode volatile* qnode_ptr;

    
    // these are the user/clh_manager's view of the universe
    struct live_handle { qnode_ptr _ptr; };
    struct dead_handle { qnode_ptr _ptr; };

    /* functions to initialize and destroy thread-local handle
       management. Any program wishing to use automatic handle
       managerment must call these two functions at the beginning and
       end of every thread's execution.

       If the user will always manage handles manually, these
       functions can be safely ignored.
     */
    static void thread_init_manager() {
	manager() = new Manager;
    }
    static void thread_destroy_manager() {
	delete manager();
    }


    /* Functions to create and destroy handles. 

       All lock operations require a handle (either dead or alive);
       both the handle manager and user use these functions to
       allocate and destroy them.

       WARNING: handles *must* be allocated on the heap because they
       may outlive both the thread and the lock using them.
     */
    static dead_handle create_handle();
    static void destroy_handle(dead_handle h);    

    // default constructor
    clh_rwlock();
    


    /* Acquire this lock in write-mode. The call will spin until all
       existing readers have called release(); any new readers will
       spin until we call release().

       WARNING: Nested reads and/or writes are unsafe and will likely cause deadlock 
    */
    void acquire_read();

    /* Acquire this lock in read-mode. The call will spin until all
       existing writers have called release() (including those still
       spinning on the lock).

       WARNING: Nested reads and/or writes are unsafe and will likely cause deadlock 
    */
    void acquire_write();

    /* Release this lock
     */
    void release();

    /* These functions are exactly like their counterparts above, but
       use automatic handle management, which vastly simplifies user
       code at the expense of some minor overhead.

       NOTE: Users should prefer automatic management unless there is
       compelling evidence of significant overhead. My profiling
       indicates that automatic management adds only 5-7% overhead to
       each lock operation (which is already extremely fast).
      
       @param seed a dead_handle returned by create_handle(). It will
       be invalidated by the call and should not be used again

       @param me a live handle to be passed to release(). It contains
       state specific to this thread-lock pair and should not be mixed
       up with other threads or locks.
     */
    live_handle acquire_write(dead_handle seed);
    live_handle acquire_read(dead_handle me);
    dead_handle release(live_handle me);

private:
    /*
      ======================================================================
      The following classes all wrap up one or two qnodes and serve mainly
      to keep track of whether a node is "live" or not.
       
      It may seem like overkill/code bloat/paranoia to represent live
      and dead nodes with classes, but it *really* helps keep things
      straight in the functions that work black magic with the shunt
      queue. Trust me on this one!
      ======================================================================
    */
    // forward refs
    class dnode;
    class pre_baton_pair;
    
    // a qnode that is currently in a queue waiting for the baton
    class lnode {
	qnode_ptr _ptr;
	friend class dnode;
    public:
	lnode() : _ptr(NULL) { }
	explicit lnode(qnode_ptr ptr) : _ptr(ptr) { }
	explicit lnode(dnode* node);
	lnode(live_handle h) : _ptr(h._ptr) { }
	qnode_ptr operator->() { return _ptr; }
	bool valid() volatile { return _ptr != NULL; }
	operator live_handle() volatile { live_handle h = {_ptr}; return h; }
	bool operator==(lnode volatile const &other) volatile const { return _ptr == other._ptr; }
	bool operator!=(lnode volatile const &other) volatile const { return !(*this == other); }
	
	dnode kill(); // turn this node into a dnode for some special reason. Caveat emptor!
	bool is_waiting();
	dnode spin();
	void pass_baton(long read_count, lnode writer);

	// for debug only; return true if a cycle
	bool print_queue() volatile;
    };

    // a qnode that is not currently waiting in a queue, and therefore safe to read from
    class dnode {
	qnode_ptr _ptr;
	friend class lnode;
    public:
	dnode() : _ptr(NULL) { }
	explicit dnode(qnode_ptr ptr) : _ptr(ptr) { }
	explicit dnode(lnode* node);
	explicit dnode(dead_handle h) : _ptr(h._ptr) { }
	qnode_ptr operator->() { return _ptr; }
	operator dead_handle() { dead_handle h = {_ptr}; return h; }
    
	lnode writer();
	pre_baton_pair join_queue(lnode &queue, bool atomic=true);
    };
    
    struct post_baton_pair {
	dnode _pred;
	lnode _me;
	post_baton_pair(dnode pred, lnode me) : _pred(pred), _me(me) { }
    };

    struct pre_baton_pair {
	lnode _pred;
	lnode _me;
	pre_baton_pair() { }
	pre_baton_pair(lnode &pred, lnode &me) : _pred(pred), _me(me) { }

	post_baton_pair spin() { return post_baton_pair(_pred.spin(), _me); }
    };

private:
    static Manager* &manager();
    
    lnode _queue;
    lnode _shunt_queue;
    
    post_baton_pair wait_for_baton(dnode seed, bool check_for_writer=true);
};
    

#endif
