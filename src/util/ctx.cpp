#include <sys/types.h>
#include <cstdlib>
#include <cassert>
#include <sys/mman.h>
#include <cstdio>
#include <map>
#include <vector>
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
MaG*/
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
    ppc64_stack_frame** owner;
};

typedef std::map<void*, ppc64_context> context_map;
typedef std::vector<std::pair<void*, ppc64_context> > context_list;
context_map ctx_live;
context_list ctx_zombies;

void ctx_swap(ppc64_stack_frame** from, ppc64_stack_frame* to);
void ctx_exit(void* rval) __attribute__((noreturn));
void ctx_exit(context_map::iterator it, void* rval) __attribute__((noreturn));

context_map::iterator ctx_current() {
    int dummy;
    // look up the context for this stack
    context_map::iterator it = ctx_live.lower_bound(&dummy);
    assert(it != ctx_live.end());
    ppc64_context ctx = it->second;
    if((int) it->second.stack_size < (char*)it->first - (char*) &dummy) {
        printf("Whoops! Stack overflow or bad context!\n");
        assert(false);
    }
    return it;
}

void ctx_yield(ppc64_stack_frame** cur) {
    ppc64_stack_frame** owner = ctx_current()->second.owner;
    ctx_swap(cur, *owner);
}


void ctx_exit(context_map::iterator it, void* rval) {
    // flag the context for deletion
    ppc64_stack_frame** owner = it->second.owner;
    it->second.rval = rval;
    ctx_zombies.push_back(*it);
    ctx_live.erase(it, it);
    
    // return to owner
    ppc64_stack_frame* cur = it->second.r1;
    ctx_swap(&cur, *owner);
    assert(false);
}

void ctx_exit(void* rval) {
    ctx_exit(ctx_current(), rval);
}

// dirty hack: this function is only ever "called" by returning from
// ctx_swap for the first time a context is scheduled. Even though
// ctx_swap won't explicinly pass the params, it doesn't clobber the
// ones passed to it so we're ok.
void ctx_start(ppc64_stack_frame** prev, ppc64_stack_frame* cur) {
    // look up our context
    context_map::iterator it = ctx_current();
    ppc64_context ctx = it->second;
    
    // exit with function's rval
    ctx_exit(it, (ctx.func)(ctx.arg));
}

ppc64_stack_frame* ctx_create(void* (*func)(void*), void* arg,
                          ppc64_stack_frame** owner, size_t stack_size=0) {
    static int const K = 1024;
    static int const M = K*K;
    
    // Zero size is a hint by the caller to use a reaonable default
    if(stack_size == 0)
        stack_size = 1*M;

    void* buffer = mmap(0, stack_size, 
                        PROT_READ|PROT_WRITE,
                        MAP_PRIVATE|MAP_ANONYMOUS|MAP_GROWSDOWN,
                        0, 0);
    // allocation failure?
    if(buffer == (void*)-1)
        return NULL;

    // reserve a stack frame -- it actually gets accessed, though only
    // the value of save_lr ends up getting used
    ppc64_stack_frame* r1 = -1 + (ppc64_stack_frame*) buffer;

    // hack! on power5 the "address" of a function is actually a
    // pointer to its address. Dereference to avoid badness...
    r1->save_lr = *(Reg*) &ctx_start;
    
    // register it for later
    ppc64_context &ctx = ctx_live[buffer];
    ctx.r1 = r1;
    ctx.stack_size = stack_size;
    ctx.func = func;
    ctx.arg = arg;
    ctx.rval = NULL;
    ctx.owner = owner;//? owner : ctx_get();
    return r1;
}

static int const COUNT = 1000000;
volatile bool a_ready = true;
volatile bool b_ready = false;

ppc64_stack_frame* ctxa;
ppc64_stack_frame* ctxb;

void* a(void*) {
    for(int i=0; i < COUNT; i++) {
        //printf("A");
        ctx_swap(&ctxa, ctxb);
    }
    ctx_exit(NULL);
}

void* b(void*) {
    for(int i=0; i < COUNT; i++) {
        //printf("B");
        ctx_swap(&ctxb, ctxa);
    }
    ctx_exit(NULL);
}

int main() {
    ppc64_stack_frame* ctx;
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
