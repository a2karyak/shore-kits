// -*- mode:c++; c-basic-offset:4 -*-
#include "util/clh_lock.h"
#include <cassert>
#include <pthread.h>
#include "util/stopwatch.h"
#include <stdio.h>
#include "util/clh_rwlock.h"

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
	
clh_lock global_lock;
pthread_mutex_t global_plock = PTHREAD_MUTEX_INITIALIZER;
pthread_rwlock_t global_rwlock = PTHREAD_RWLOCK_INITIALIZER;
void test_shared_prlock() {
    for(long i=0; i < local_count; i++) {
	pthread_rwlock_rdlock(&global_rwlock);
	pthread_rwlock_unlock(&global_rwlock);
    }
}
void test_shared_pwlock() {
    for(long i=0; i < local_count; i++) {
	pthread_rwlock_wrlock(&global_rwlock);
	pthread_rwlock_unlock(&global_rwlock);
    }
}
clh_rwlock global_clh_rwlock;
void test_shared_rlock() {
    for(long i=0; i < local_count; i++) {
	global_clh_rwlock.acquire_read();
	global_clh_rwlock.release();
    }
}
void test_shared_wlock() {
    for(long i=0; i < local_count; i++) {
	global_clh_rwlock.acquire_write();
	global_clh_rwlock.release();
    }
}
void test_shared_pthreads() {
    for(long i=0; i < local_count; i++) {
	pthread_mutex_lock(&global_plock);
	pthread_mutex_unlock(&global_plock);
    }
}
void test_shared_auto() {
    for(long i=0; i < local_count; i++) {
	global_lock.acquire();
	global_lock.release();
    }
}
void test_shared_manual() {
    clh_lock::dead_handle h = clh_lock::create_handle();
    for(long i=0; i < local_count; i++) {
	h = global_lock.release(global_lock.acquire(h));
    }
}

void test_independent() {
    clh_lock local_lock;
    for(long i=0; i < local_count; i++) {
	local_lock.acquire();
	local_lock.release();
    }
}

int main() {
    pthread_t tids[THREADS];

    for(int j=0; j < 3; j++) {
    printf("Per-thread acquire-release cost for 1..%d threads (in usec)\n", THREADS);
    for(int k=1; k <= THREADS; k++) {
	local_count = COUNT/k;
	for(long i=0; i < k; i++)
	    pthread_create(&tids[i], NULL, &run, (void*) &test_shared_wlock);
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

