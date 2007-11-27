// -*- mode:c++; c-basic-offset:4 -*-
#include "util/clh_lock.h"
#include <cassert>

#include "util/stopwatch.h"
#include <stdio.h>

static int const THREADS = 32;
static long const COUNT = (1l << 28)/THREADS;

extern "C" void* run(void* arg) {
    clh_lock::thread_init();
    
    stopwatch_t timer;
    ((void (*)()) arg)();
     
    union {
	double d;
	void* vptr;
    } u = {timer.time()};

     clh_lock::thread_destroy();
     return u.vptr;
     }
	
clh_lock global_lock;
void test_shared() {
    for(long i=0; i < COUNT; i++) {
	global_lock.acquire();
	global_lock.release();
    }
}

void test_independent() {
    clh_lock local_lock;
    for(long i=0; i < COUNT; i++) {
	local_lock.acquire();
	local_lock.release();
    }
}

int main() {
    pthread_t tids[THREADS];

    for(int k=1; k <= THREADS; k*=2) {
	for(long i=0; i < k; i++)
	    pthread_create(&tids[i], NULL, &run, (void*) &test_shared);
	union {
	    void* vptr;
	    double d;
	} u;
	double total = 0;
	for(long i=0; i < k; i++) {
	    pthread_join(tids[i], &u.vptr);
	    total += u.d;
	}
	printf("%d %.3lf us\n", k, 1e6*total/COUNT/k);
    }
    return 0;
}

