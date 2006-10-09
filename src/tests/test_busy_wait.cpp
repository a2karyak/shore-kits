/* -*- mode:C++; c-basic-offset:4 -*- */

#include <cstdlib>
#include <cstdio>
#include <cassert>

#include "util.h"
#include "tests/common/cpu_bind.h"
#include "tests/common/busy_wait.h"



int main(int argc, char* argv[]) {


    // parse command-line args
    if ( argc < 3 ) {
        printf("Usage: %s <num seconds> <cpu index>\n", argv[0]);
        return -1;
    }

    const char* num_seconds_str = argv[1];
    int num_seconds = atoi(num_seconds_str);
    if (num_seconds <= 0) {
        printf("Invalid number of seconds %s\n", num_seconds_str);
        return -1;
    }

    const char* cpu_index_str = argv[2];
    int cpu_index = atoi(cpu_index_str);
    if (cpu_index < 0) {
        printf("Invalid cpu index %s\n", cpu_index_str);
        return -1;
    }
  

    // set wake-up in num_seconds seconds
    if (alarm(num_seconds) != 0) {
        printf("alarm() failed\n");
        return -1;
    }

    bind_to_cpu(cpu_index);
    busy_wait();

    return 0;
}
