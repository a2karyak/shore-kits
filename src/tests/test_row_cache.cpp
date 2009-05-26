// -*- mode:c++; c-basic-offset:4 -*-

#include <cassert>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "sm/shore/shore_row_cache.h"
#include "workload/tpcc/shore_tpcc_schema.h"
#include "workload/tpcc/shore_tpcc_schema_man.h"


// only works on sparc...
#include "sys/time.h"

using namespace shore;
using namespace tpcc;


static int const THREADS = 32;
static int const ITERS = 5;
static long const COUNT = 1l << 20;


volatile bool ready;

warehouse_t _warehouse_desc(string("baseline"));
warehouse_man_impl* _pwh_man;

#include <unistd.h>
extern "C" void* run(void* arg) 
{
    int iter = (int)(long)arg;
    while(!ready);
    stopwatch_t timer;
    int k = 0;

    // DO WORK()
    for (int j=0; j<(iter+ITERS); j++) {
#if 1
        row_impl<warehouse_t>* prwh = _pwh_man->get_tuple();
        prwh->set_value(0, j);
        prwh->get_value(0, k);
        //        fprintf( stdout, "%d (%d)...\n", iter, k);
        _pwh_man->give_tuple(prwh);
#else
#if 1
        row_impl<warehouse_t> rwh(&_warehouse_desc);
        rwh.set_value(0, j);
        rwh.get_value(0, k);
#else
        row_impl<warehouse_t>* prwh = new row_impl<warehouse_t>(&_warehouse_desc);
        prwh->set_value(0, j);
        prwh->get_value(0, k);
        delete prwh;
#endif
#endif
    }

    union {
	double d;
	void* vptr;
    } u = {timer.time()};

    return (NULL);
}

#include <string.h>
#include <unistd.h>
int main() 
{
    thread_init();

    pthread_t tids[THREADS];
    int thread_delta = 5;

    _pwh_man = new warehouse_man_impl(&_warehouse_desc);

    fprintf(stderr, "Testing tuple-cache with (%d) threads\n", THREADS);

    for (int j=0; j<ITERS; j++) {

        for(int k=1; k <= THREADS; k+=thread_delta) {
            fprintf(stderr, "%d\n", k);
            ready = false; // stop threads

            for(int i=0; i < k; i++)
                pthread_create(&tids[i], NULL, &run, (void*)i);

            union {
                void* vptr;
                double d;
            } u;
            usleep(100);
            ready = true; // start threads
            stopwatch_t timer;
            double total = 0;
            for(long i=0; i < k; i++) {
                pthread_join(tids[i], &u.vptr);
                total += u.d;
            }
            double wall = 1e6*timer.time()/k;
            fprintf( stdout, "wall=%.2f\n", wall);
        }
    }

    _pwh_man->print_cache_stats();

    delete (_pwh_man);

    return (0);
}
