/* -*- mode:C++; c-basic-offset:4 -*- */

#include <cstdlib>
#include <cstdio>
#include <cassert>

#include "util.h"
#include "tests/common/cpu_bind.h"
#include "tests/common/busy_wait.h"



#define NUMCPUS 4
int cpu_indices[NUMCPUS] = { 0, 1, 2, 3 };



void* thread_main(void* arg) {

    int* cpu_index_addr = (int*)arg;
    int  cpu_index = *cpu_index_addr;

    bind_to_cpu(cpu_index);

    printf("Bound to cpu %d\n", cpu_index);

    busy_wait();
    return NULL;
}



int main(int argc, char* argv[]) {


    // parse command-line args
    if ( argc < 2 ) {
        printf("Usage: %s <num seconds>\n", argv[0]);
        return -1;
    }

    const char* num_seconds_str = argv[1];
    int num_seconds = atoi(num_seconds_str);
    if (num_seconds <= 0) {
        printf("Invalid number of seconds %s\n", num_seconds_str);
        return -1;
    }

    // set wake-up in num_seconds seconds
    if (alarm(num_seconds) != 0) {
        printf("alarm() failed\n");
        return -1;
    }


    // since we have the root thread, we can avoid creating one.
    int i;
    for (i = 0; i < (NUMCPUS-1); i++) {
        pthread_t tid;
        int ret = pthread_create(&tid, NULL, thread_main, &cpu_indices[i]);
        assert(!ret);
    }


    // put the root thread to work as well...
    thread_main(&cpu_indices[i]);


    return 0;
}
