/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/thread.h"

#include "engine/dispatcher/dispatcher_cpu_set.h"
#include "engine/dispatcher/dispatcher_cpu_set_struct.h"
#include "tests/common/cpu_bind.h"


void bind_to_cpu(int cpu_index) {

    // get the set of CPUs on this system
    struct dispatcher_cpu_set_s cpu_set;
    dispatcher_cpu_set_init(&cpu_set);

    // try binding to first CPU
    dispatcher_cpu_t bind_cpu = dispatcher_cpu_set_get_cpu(&cpu_set, cpu_index);
    dispatcher_cpu_bind_self(bind_cpu);

    dispatcher_cpu_set_finish(&cpu_set);
}
