/* -*- mode:C++; c-basic-offset:4 -*- */

#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <ucontext.h>
#include <signal.h>

#include "util.h"
#include "tests/common/cpu_bind.h"
#include "tests/common/busy_wait.h"


/* flag cannot be local or it gets restored as well! */

int g_switched = 0;


extern "C" void* switcher_main(int arg)
{
    TRACE(TRACE_ALWAYS, "IN SWITCHER WITH ARG = %d\n", arg);
    return NULL;
}


int main(int, char**) {

    ucontext_t root;
    ucontext_t switcher;
    

    /* makecontext() will create a new context C. makecontext()
       requires that C's uc_link pointer be set to "the context that
       will be run when C exits from func()." In other words, we must
       have an initialized context ready when we call
       makecontext(). Since the only other way to create a context is
       by using getcontext(), we have the following choices:

       - Return to the root context (the context that was running
         before we made any context switches). This is the preferred
         approach.
        
       - Exit the kernel thread (I guess with pthread_exit()).

       - Exit the program (with the exit() system call).
    */
    
    int getroot = getcontext(&root);
    assert(getroot != -1);
    int getswitcher = getcontext(&switcher);
    assert(getswitcher != -1);


    if (!g_switched) {
        g_switched = 1;

        /* create a new context */
        long stack_size = SIGSTKSZ;
        int* stack = (int*)malloc(stack_size);
        if (stack == NULL){
            TRACE(TRACE_ALWAYS, "malloc() failed\n");
            return -1;
        }

        switcher.uc_stack.ss_sp    = stack;
        switcher.uc_stack.ss_flags = 0;
        switcher.uc_stack.ss_size  = stack_size;
        switcher.uc_link = &root;

        makecontext(&switcher, (void (*)())switcher_main, 1, 1021);

        /* the switcher context should now be ready... */
        /* Let's try running it with setcontext() */
        TRACE(TRACE_ALWAYS, "Going to run switcher\n");
        int setret = setcontext(&switcher);
        assert(setret != -1);

        TRACE(TRACE_ALWAYS, "Continued after setcontext!\n");
        assert(0);
    }


    TRACE(TRACE_ALWAYS, "Back in root thread!\n");
    TRACE(TRACE_ALWAYS, "Done with program\n");
    return 0;
}
