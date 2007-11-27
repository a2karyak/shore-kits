// -*- mode:c++; c-basic-offset:4 -*-
#include <pthread.h>
#include <unistd.h>
#include "util/clh_rwlock.h"
#ifdef __SUNPRO_CC
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#else
#include <cstdlib>
#include <cstdio>
#include <cassert>
#endif

#include "util/stopwatch.h"


static int const THREADS=32;
static long const COUNT = 1l << 20;
static long local_count; // how many times to loop?
static long local_wmask; // how often to write?

clh_rwlock global_lock;

extern "C" void* test_shunt(void* arg) {
    clh_rwlock::thread_init_manager();
    if((long) arg == 0) {
	global_lock.acquire_read();
	sleep(10);
	global_lock.release();
    }
    else {
	for(int i=0; i < local_count; i++) {
	    global_lock.acquire_write();
	    global_lock.release();
	}
    }
    clh_rwlock::thread_destroy_manager();
    return NULL;
}
extern "C" void* test_shared_rwlock(void* arg) {
    clh_rwlock::thread_init_manager();
    stopwatch_t timer;
    long offset = 31*(long) arg;
    for(long i=0; i < local_count; i++) {
	switch(local_wmask) {
	case 0: global_lock.acquire_write();break;
	case 1: global_lock.acquire_read(); break;
	default:
	    if((i+offset)&local_wmask == 0) 
		global_lock.acquire_write();
	    else
		global_lock.acquire_read();
	    break;
	}
	global_lock.release();
    }
    union {
	double d;
	void* vptr;
    } u = {timer.time()};
    clh_rwlock::thread_destroy_manager();
//    fprintf(stderr, "Thread %ld finished\n", (long) arg);
    return u.vptr;
}

int main() {
    pthread_t tids[THREADS];

    for(int wmask=0; wmask < (1 << 10); wmask=(wmask+1)*2-1) {
	printf("Starting round %d...\n", wmask);
#ifdef DEBUG_MODE
	history.clear();
#endif
	local_wmask = wmask;
	for(int k=1; k <= THREADS; k*=2) {
	    local_count = COUNT/THREADS;
	    for(long i=0; i < k; i++)
		pthread_create(&tids[i], NULL, &test_shared_rwlock, (void*) i);
	    union {
		void* vptr;
		double d;
	    } u;
	    double total = 0;
	    for(long i=0; i < k; i++) {
		pthread_join(tids[i], &u.vptr);
		total += u.d;
	    }
	    printf("%d %.3lf us\n", k, 1e6*total/local_count/k);
	}
    }
    return 0;
}
