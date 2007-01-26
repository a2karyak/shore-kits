/* -*- mode:C++; c-basic-offset:4 -*- */

#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <ucontext.h>

#include "util.h"
#include "tests/common/cpu_bind.h"
#include "tests/common/busy_wait.h"


void start_thread_main(ucontext_t* switch_thread)
{
    TRACE(TRACE_ALWAYS, "STARTING start_thread_main\n");
}


void switch_thread_main(void)
{
    TRACE(TRACE_ALWAYS, "STARTING switch_thread_main\n");
}


int main(int, char**) {

#if 0
    ucontext_t start_thread, switch_thread;
    
    TRACE(TRACE_ALWAYS, "Going to call getcontext(&start_thread)\n");
    getcontext(&start_thread);
    TRACE(TRACE_ALWAYS, "Called getcontext(&start_thread)\n");

    getcontext(&switch_thread);
    makecontext(&switch_thread, switch_thread_main, 0);
    makecontext(&start_thread, start_thread_main, 1, &switch_thread);
#endif    

    return 0;
}
