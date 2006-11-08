#define USE_VALGRIND
#include "util/ctx.h"
#include "util/stopwatch.h"
#include <cstdio>

static int const COUNT = 10;
volatile bool a_ready = true;
volatile bool b_ready = false;

ctx_handle* ctxa;
ctx_handle* ctxb;

void* a(void*) {
    for(int i=0; i < COUNT; i++) {
        printf("A");
        ctx_swap(&ctxa, ctxb);
    }
    throw 1;
    ctx_exit(NULL);
}

void* b(void*) {
    for(int i=0; i < COUNT; i++) {
        printf("B");
        ctx_swap(&ctxb, ctxa);
    }
    ctx_exit(NULL);
}

int main() {
    ctx_handle* ctx;
    stopwatch_t timer;
    ctxa = ctx_create(a, NULL, &ctx);
    ctxb = ctx_create(b, NULL, &ctx);
    ctx_swap(&ctx, ctxa);
    double ms = timer.time_ms();
    int total = (COUNT+1)*2;
    printf("Context switches: %d\n"
                      "Total Time: %.3f ms\n"
                      "Time/switch: %.3f us\n",
                      total, ms, ms/total*1000);
}
