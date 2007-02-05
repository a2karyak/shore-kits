/* -*- mode:C++; c-basic-offset:4 -*- */

#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <ucontext.h>

#include "util.h"
#include "tests/common/cpu_bind.h"
#include "tests/common/busy_wait.h"


/* flag cannot be local or it gets restored as well! */

int g_switched = 0;


int main(int, char**) {

    ucontext_t thread;
    
    TRACE(TRACE_ALWAYS, "Going to call getcontext(&thread)\n");
    int getret = getcontext(&thread);
    assert(getret != -1);
    TRACE(TRACE_ALWAYS, "Called getcontext(&thread). Got %d\n", getret);
    TRACE(TRACE_ALWAYS, "g_switched = %d\n", g_switched);

    if (!g_switched) {
        g_switched = 1;
        int setret = setcontext(&thread);
        assert(setret != -1);
        assert(0);
    }

    TRACE(TRACE_ALWAYS, "Done with program\n");
    return 0;
}
