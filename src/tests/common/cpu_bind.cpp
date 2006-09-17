/* -*- mode:C++; c-basic-offset:4 -*- */

#include "scheduler.h"
#include "tests/common/cpu_bind.h"


void bind_to_cpu(int cpu_index) {

    // get the set of CPUs on this system
    struct scheduler::cpu_set_s cpu_set;
    scheduler::cpu_set_init(&cpu_set);

    // try binding to first CPU
    scheduler::cpu_t bind_cpu = scheduler::cpu_set_get_cpu(&cpu_set, cpu_index);
    scheduler::cpu_bind_self(bind_cpu);

    scheduler::cpu_set_finish(&cpu_set);
}
