/* -*- mode:C++; c-basic-offset:4 -*- */

#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <unistd.h>
#include <ucontext.h>
#include <signal.h>

#include "util.h"
#include "tests/common/cpu_bind.h"
#include "tests/common/busy_wait.h"


#define NUM_BUFFER_ENTRIES 10


/* shared data */
int running = 1;
int buffer[NUM_BUFFER_ENTRIES];
int buffer_entry_count = 0;


/* contexts */

ucontext_t producer, consumer;



extern "C" void* producer_main(int arg)
{
    static int gen = 100;

    int i;
    for (i = 0; i < 5; i++) {

        TRACE(TRACE_ALWAYS, "Running producer with arg = %d\n", arg);

        /* Fill the buffer */
        while (buffer_entry_count < NUM_BUFFER_ENTRIES) {
            buffer[buffer_entry_count] = gen;
            buffer_entry_count++;
            gen++;
        }

        /* Run consumer */
        int swapret = swapcontext(&producer, &consumer);
        assert(swapret != -1);

        /* error checking */
        /* Ensure that all buffer items have been consumed */
        assert(buffer_entry_count == 0);
    }

    running = 0;

    return NULL;
}


int main(int, char**) {

    int getcons = getcontext(&consumer);
    assert(getcons != -1);
    int getprod = getcontext(&producer);
    assert(getprod != -1);


    /* create a new context */
    long stack_size = SIGSTKSZ;
    int* stack = (int*)malloc(stack_size);
    if (stack == NULL){
        TRACE(TRACE_ALWAYS, "malloc() failed\n");
        return -1;
    }
    
    producer.uc_stack.ss_sp    = stack;
    producer.uc_stack.ss_flags = 0;
    producer.uc_stack.ss_size  = stack_size;
    producer.uc_link = &consumer;

    makecontext(&producer, (void (*)())producer_main, 1, 1021);

    while (1) {


        /* Run producer to create more data... */
        int swapret = swapcontext(&consumer, &producer);
        assert(swapret != -1);
        if (!running)
            break;


        /* Run consumer to empty out the buffer. */
        TRACE(TRACE_ALWAYS, "Running consumer\n");

        /* Ensure that buffer is full */
        assert(buffer_entry_count == NUM_BUFFER_ENTRIES);

        int i;
        for (i = 0; i < buffer_entry_count; i++) {
            TRACE(TRACE_ALWAYS, "Removed %d from position %d\n",
                  buffer[i],
                  i);
        }
        buffer_entry_count = 0;


        /* error checking */
        sleep(1);
    }


    /* should never reach here */
    TRACE(TRACE_ALWAYS, "Done with program\n");
    return 0;
}
