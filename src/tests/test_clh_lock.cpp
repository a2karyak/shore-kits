// -*- mode:c++; c-basic-offset:4 -*-
#include "util/clh_lock.h"
#include <cassert>
#include <pthread.h>
#include <stdlib.h>
#include "util/stopwatch.h"
#include <stdio.h>
#include "util/clh_rwlock.h"

// only works on sparc...
#include "sys/time.h"


static long const ONE_WRITER = 0x1;
static long const ONE_READER = 0x100;
static long const WRITE_MASK = 0xff;
static long const READ_MASK = 0xff00;
struct fast_rwlock {
    struct qnode;
    typedef qnode volatile* qnode_ptr;
    struct qnode {
	qnode_ptr _pred;
	bool volatile _waiting;
	qnode() : _pred(NULL), _waiting(false) { }
    };


    qnode_ptr volatile _tail;
    uint64_t volatile _state;

    fast_rwlock() : _tail(new qnode), _state(0) { }
    ~fast_rwlock() { delete _tail; }

    void spin_queue(qnode_ptr pred) {
	stopwatch_t timer;
	long start = timer.now();
	long now;
	while(pred->_waiting) {
	    //	    assert((now=timer.now()) < start + 10000);
	}
	membar_consumer();
    }
    void spin_state(long mask, long expected) {
	stopwatch_t timer;
	long start = timer.now();
	long now;
	while((_state & mask) != expected) {
	    //=	    assert((now=timer.now()) < start + 10000);
	}
    }
    qnode_ptr acquire_write(qnode_ptr me) {
	assert(_state < 0x10000);
	static int const WRITE_CAS_ATTEMPTS = 1;
    
	// first, try to CAS directly
	for(int i=0; i < WRITE_CAS_ATTEMPTS; i++) {
	    if(atomic_cas_64(&_state, 0, ONE_WRITER) == 0) {
	assert(_state < 0x10000);
		me->_pred = NULL; // indicates the CAS succeeded
		membar_enter();
		return me;
	    }
	}
    
	// enqueue
	me->_waiting = true;
	membar_producer(); // finish updates to i *before* it hits the queue
	qnode_ptr pred = (qnode_ptr) atomic_swap_ptr(&_tail, (void*) me);
    
	// wait our turn
	spin_queue(me->_pred = pred);
    
	/* stake our claim so readers stop grabbing the lock. At most one
	   other writer might be ahead of us at this point -- the one
	   whose CAS succeeded between my last reader calling release()
	   and me finishing this atomic add. All other potential writers
	   will be in the queue behind me, and haven't staked their claims
	   yet.
	*/
	atomic_add_64(&_state, ONE_WRITER);
	assert(_state < 0x10000);

	// now spin until we have the lock
	spin_state(-1, ONE_WRITER);

	// no need to pass the baton to our successor quite yet...
	membar_enter();
	return me;
    }
  
    qnode_ptr release_write(qnode_ptr me) {
	assert(_state < 0x10000);
	qnode_ptr pred = me->_pred;
    
	membar_exit();
	long new_state = atomic_add_64_nv(&_state, -ONE_WRITER);
	assert(_state < 0x10000);
	if(pred) {
	    /* we are in the queue - pass the baton to our successor
	       (they may very well block if there's another writer
	       around, but we have to wake them up any way or risk
	       deadlock.
	     */
	    me->_waiting = false;
	    me = pred;
	}
	return me;
    }

    qnode_ptr acquire_read(qnode_ptr me) {
	assert(_state < 0x10000);
	static int const READ_CAS_ATTEMPTS = 3;
    
	// first, try to CAS directly
	for(int i=0; i < READ_CAS_ATTEMPTS; i++) {
	    // fail immediately if there's a writer
	    long old_state = _state & READ_MASK;
	    if(atomic_cas_64(&_state, old_state, old_state+ONE_READER) == old_state) {
	assert(_state < 0x10000);
		me->_pred = NULL; // indicates the CAS succeeded
		membar_enter();
		return me;
	    }
	}
    
	// enqueue
	me->_waiting = true;
	membar_producer(); // finish updates to i *before* it hits the queue
	qnode_ptr pred = (qnode_ptr) atomic_swap_ptr(&_tail, (void*) me);
    
	// wait our turn
	spin_queue(me->_pred = pred);
    
	// stake our claim 
	atomic_add_64(&_state, ONE_READER);
	assert(_state < 0x10000);

	// now spin until we have the lock
	spin_state(WRITE_MASK, 0);
	membar_enter();

	// pass the baton to our successor
	me->_waiting = false;
	pred->_pred = pred; // anything but NULL
	return pred;
    }

    qnode_ptr release_read(qnode_ptr me) {
	assert(_state < 0x10000);
	// we already passed the baton
	membar_exit();
	atomic_add_64(&_state, -ONE_READER);
	assert(_state < 0x10000);
	return me;
    }
};

static int const THREADS = 32;
static long const COUNT = 1l << 20;

long local_count;
extern "C" void* run(void* arg) {
    clh_lock::thread_init_manager();
    clh_rwlock::thread_init_manager();
    
    
    stopwatch_t timer;
    ((void (*)()) arg)();
     
    union {
	double d;
	void* vptr;
    } u = {timer.time()};

     clh_lock::thread_destroy_manager();
     clh_rwlock::thread_destroy_manager();
     return u.vptr;
}

static long const DELAY_NS = 1000; // 1usec
static int const LINES_TOUCHED = 2;
void ncs(long delay_ns=DELAY_NS) {
    // spin for 1 usec
#ifdef __SUNPRO_CC
    hrtime_t start = gethrtime();
    while(gethrtime() < start + delay_ns);
#else
    // round up to the neares us
    long delay_us = (delay_ns + 999)/1000;
    stopwatch_t timer;
    long start = timer.now();
    while(timer.now() < start+DELAY_US);
#endif
}
void cs(long lines=LINES_TOUCHED) {
    // assume  cache lines no larger than 128-byte
    static int const MAX_LINES = 100;
    static int const LINE_SIZE = 64;
    static int volatile data[MAX_LINES*LINE_SIZE/sizeof(int)];

    for(int i=0; i < lines; i++)
	data[i*LINE_SIZE];
    
}
clh_lock global_lock;
pthread_mutex_t global_plock = PTHREAD_MUTEX_INITIALIZER;
pthread_rwlock_t global_rwlock = PTHREAD_RWLOCK_INITIALIZER;
void test_shared_prlock() {
    for(long i=0; i < local_count; i++) {
	ncs();
	pthread_rwlock_rdlock(&global_rwlock);
	cs();
	pthread_rwlock_unlock(&global_rwlock);
    }
}
void test_shared_pwlock() {
    for(long i=0; i < local_count; i++) {
	ncs();
	pthread_rwlock_wrlock(&global_rwlock);
	cs();
	pthread_rwlock_unlock(&global_rwlock);
    }
}
clh_rwlock global_clh_rwlock;
void test_shared_rlock() {
    for(long i=0; i < local_count; i++) {
	ncs();
	global_clh_rwlock.acquire_read();
	cs();
	global_clh_rwlock.release();
    }
}
void test_shared_wlock() {
    for(long i=0; i < local_count; i++) {
	ncs();
	global_clh_rwlock.acquire_write();
	cs();
	global_clh_rwlock.release();
    }
}
void test_shared_pthreads() {
    for(long i=0; i < local_count; i++) {
	ncs();
	pthread_mutex_lock(&global_plock);
	cs();
	pthread_mutex_unlock(&global_plock);
    }
}
void test_shared_auto() {
    for(long i=0; i < local_count; i++) {
	ncs();
	global_lock.acquire();
	cs();
	global_lock.release();
    }
}
void test_shared_manual() {
    clh_lock::dead_handle h = clh_lock::create_handle();
    for(long i=0; i < local_count; i++) {
	ncs();
	clh_lock::live_handle lh = global_lock.acquire(h);
	cs();
	h = global_lock.release(lh);
    }
}

void test_independent() {
    clh_lock local_lock;
    for(long i=0; i < local_count; i++) {
	ncs();
	local_lock.acquire();
	cs();
	local_lock.release();
    }
}

fast_rwlock global_fast_rwlock;
void test_fast_rlock() {
    fast_rwlock::qnode_ptr me = new fast_rwlock::qnode;
    for(long i=0; i < local_count; i++) {
	//ncs();
	me = global_fast_rwlock.acquire_read(me);
	//cs();
	me = global_fast_rwlock.release_read(me);
    }
}
void test_fast_wlock() {
    fast_rwlock::qnode_ptr me = new fast_rwlock::qnode;
    for(long i=0; i < local_count; i++) {
	//ncs();
	me = global_fast_rwlock.acquire_write(me);
	//cs();
	me = global_fast_rwlock.release_write(me);
    }
}
void test_fast_rwlock() {
    static __thread int dummy = 0;
    long offset = ((long) &dummy >> 3) & 31;
    
    fast_rwlock::qnode_ptr me = new fast_rwlock::qnode;
    for(long i=0; i < local_count; i++) {
	if(((i + offset) & 3) == 0) {
	//ncs();
	me = global_fast_rwlock.acquire_write(me);
	//cs();
	me = global_fast_rwlock.release_write(me);
	}
	else {
	me = global_fast_rwlock.acquire_read(me);
	//cs();
	me = global_fast_rwlock.release_read(me);
	}
    }
}

#define CHOOSE_TEST(func)			\
    void *test_func = (void*) &func;			\
    char const* test_name = #func
    
int main() {
    pthread_t tids[THREADS];

    // ====================================================================== 
    CHOOSE_TEST(test_fast_rwlock);
    // ======================================================================
    for(int j=0; j < 3; j++) {
	printf("Per-thread cost for 1..%d threads using %s (in usec)\n", THREADS, test_name);
    for(int k=1; k <= THREADS; k++) {
	local_count = COUNT/k;
	for(long i=0; i < k; i++)
	    pthread_create(&tids[i], NULL, &run, test_func);
	union {
	    void* vptr;
	    double d;
	} u;
	double total = 0;
	for(long i=0; i < k; i++) {
	    pthread_join(tids[i], &u.vptr);
	    total += u.d;
	}
	printf("%.3lf\n", 1e6*total/COUNT/k);
    }
    }
    return 0;
}

