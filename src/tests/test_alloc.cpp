/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util/pool_alloc.h"
#include <vector>
#include <algorithm>
#include "util/stopwatch.h"
#ifdef __SUNPRO_CC
#include <stdlib.h>
#include <stdio.h>
#else
#include <cstdlib>
#include <cstdio>
#endif

struct malloc_alloc {
    void* alloc(int size) { return ::malloc(size); }
    void free(void* ptr) { ::free(ptr); }
};

malloc_alloc ma;
pool_alloc pa;

// just try to allocate and free a bunch in a row...
void test1() {
    std::vector<void*> pointers;
    static int const ITERATIONS = 1;

    // find out how much the shuffles cost
    pointers.resize(1000);
    stopwatch_t timer;
    for(int i=0; i < ITERATIONS; i++) {
	std::random_shuffle(pointers.begin(), pointers.end());
    }
    double shuffle_time = timer.time_ms();
    for(int j=0; j < ITERATIONS;  j++) {
	pointers.clear();
	for(int i=0; i < 1000; i++)
	    pointers.push_back(pa.alloc(10));

	std::random_shuffle(pointers.begin(), pointers.end());
	for(unsigned i=0; i < pointers.size(); i++)
	    pa.free(pointers[i]);
    }
    double t = timer.time_ms();
    fprintf(stderr, "Cycled 10 bytes 1M times in %.3f ms (%.3f w/o shuffle)\n", t, t-shuffle_time);
}

int main() {
    test1();

    return 0;
}
