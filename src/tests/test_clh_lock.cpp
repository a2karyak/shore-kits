// -*- mode:c++; c-basic-offset:4 -*-
#include "util/clh_lock.h"
#include <cassert>
#include <pthread.h>
#include "util/stopwatch.h"
#include <stdio.h>

static int const THREADS = 32;
static long const COUNT = 1l << 20;

long local_count;
extern "C" void* run(void* arg) {
    clh_lock::thread_init_manager();
    
    stopwatch_t timer;
    ((void (*)()) arg)();
     
    union {
	double d;
	void* vptr;
    } u = {timer.time()};

     clh_lock::thread_destroy_manager();
     return u.vptr;
     }
	
clh_lock global_lock;
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
	    pthread_create(&tids[i], NULL, &run, (void*) &test_shared_manual);
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

