#include <sys/types.h>
#include <cstdlib>
#include <cassert>
#include <sys/mman.h>
#include <cstdio>
#include "stopwatch.h"

typedef __uint64_t Reg;

/** http://www.freestandards.org/spec/ELF/ppc64/PPC-elf64abi.html#STACK
    High Address

              +-> Back chain
              |   Floating point register save area
              |   General register save area
              |   VRSAVE save word (32-bits)
              |   Alignment padding (4 or 12 bytes)
              |   Vector register save area (quadword aligned)
              |   Local variable space
              |   Parameter save area    (SP + 48)
              |   TOC save area          (SP + 40)
              |   link editor doubleword (SP + 32)
              |   compiler doubleword    (SP + 24)
              |   LR save area           (SP + 16)
              |   CR save area           (SP + 8)
    SP  --->  +-- Back chain             (SP + 0)

    Low Address
*/
struct ppc64_stack_frame {
    Reg back_chain;
    Reg save_cr;
    Reg save_lr;
    Reg reserved_compiler;
    Reg reserved_linker;
    Reg save_toc;
    Reg save_params[];
} __attribute__((aligned(16)));

struct ppc64_context {
    ppc64_stack_frame* r1;
    size_t stack_size;
    void* (*func)(void*);
    void* arg;
    void* rval;
    // return to the owner context when this context exits
    ppc64_context* owner;
} __attribute__((aligned(16)));

void ctx_swap(ppc64_context* from, ppc64_context* to);



// dirty hack: this function is only ever "called" by returning from
// ctx_swap for the first time a context is scheduled. Even though
// ctx_swap won't explicinly pass the params, it doesn't clobber the
// ones passed to it so we're ok.
void ctx_start(ppc64_context* prev, ppc64_context* cur) {
    cur->rval = (cur->func)(cur->arg);
    // return to whoever started 
    ctx_swap(cur, cur->owner);
    assert(false);
}


ppc64_context* ctx_get();

ppc64_context* ctx_create(void* (*func)(void*), void* arg,
                          size_t stack_size=0, ppc64_context* owner=NULL) {
    static int const K = 1024;
    static int const M = K*K;
    
    // Zero size is a hint by the caller to use a reaonable default
    if(stack_size == 0)
        stack_size = 1*M;

    char* buffer = (char*) mmap(0, stack_size, 
                                PROT_READ|PROT_WRITE,
                                MAP_PRIVATE|MAP_ANONYMOUS|MAP_GROWSDOWN,
                                0, 0);
    // allocation failure?
    if(buffer == (void*)-1)
        return NULL;
    
    // reserve the very top of the stack for the context itself (makes
    // munmap() much easier to call). Stack must remain quadword-aligned.
    ppc64_context* ctx = -1 + (ppc64_context*) buffer;
    
    // reserve a stack frame as well -- it actually gets accessed
    ppc64_stack_frame* r1 = -1 + (ppc64_stack_frame*) ctx;

    // hack! the "address" of a function is actually a pointer to
    // its address. Dereference to avoid badness...
    asm("ld %0, 0(%1)" : "=b"(r1->save_lr) : "b"(&ctx_start));
    
    // populate
    ctx->r1 = r1;
    ctx->stack_size = stack_size;
    ctx->func = func;
    ctx->arg = arg;
    ctx->rval = NULL;
    ctx->owner = owner;//? owner : ctx_get();

    // TODO: register with global scheduler
    return ctx;
}

static int const COUNT = 10;
volatile bool a_ready = true;
volatile bool b_ready = false;

ppc64_context* ctxa;
ppc64_context* ctxb;

void* a(void*) {
    for(int i=0; i < COUNT; i++) {
        printf("A");
        ctx_swap(ctxa, ctxb);
        a_ready = false;
        b_ready = true;
    }
    return NULL;
}

void* b(void*) {
    for(int i=0; i < COUNT; i++) {
        printf("B");
        ctx_swap(ctxb, ctxa);
        b_ready = false;
        a_ready = true;
    }
    return NULL;
}

int main() {
    ppc64_context ctx;
    stopwatch_t timer;
    ctxa = ctx_create(a, NULL, 0, &ctx);
    ctxb = ctx_create(b, NULL, 0, &ctx);
    ctx_swap(&ctx, ctxa);
    double ms = timer.time_ms();
    int total = (COUNT+1)*2;
    printf("Context switches: %d\n"
                      "Total Time: %.3f ms\n"
                      "Time/switch: %.3f us\n",
                      total, ms, ms/total*1000);
}
