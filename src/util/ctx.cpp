#include <sys/types.h>
#include <cstdlib>
#include <cassert>
#include <sys/mman.h>
#include <cstdio>
#include <map>
#include <vector>
#include "util/ctx.h"
#include "util/exception.h"
#include "valgrind.h"

struct ctx_context {
    ctx_handle* self;
    size_t stack_size;
    void* (*func)(void*);
    void* arg;
    void* rval;
    ctx_handle** owner;
};

typedef std::map<void*, ctx_context> context_map;
typedef std::vector<std::pair<void*, ctx_context> > context_list;
context_map ctx_live;
context_list ctx_zombies;

static
void ctx_exit(context_map::iterator it, void* rval) __attribute__((noreturn));

static
context_map::iterator ctx_current() {
    // look up the context for this stack
    int dummy;
    context_map::iterator it = ctx_live.lower_bound(&dummy);
    assert(it != ctx_live.end());
    if((int) it->second.stack_size < (char*)it->first - (char*) &dummy) {
        printf("Whoops! Stack overflow or bad context!\n");
        assert(false);
    }
    return it;
}

void ctx_yield(ctx_handle** cur) {
    ctx_handle** owner = ctx_current()->second.owner;
    ctx_swap(cur, *owner);
}


static
void ctx_exit(context_map::iterator it, void* rval) {
    // flag the context for deletion
    ctx_handle** owner = it->second.owner;
    ctx_handle* self = it->second.self;
    it->second.rval = rval;
    ctx_zombies.push_back(*it);
    ctx_live.erase(it, it);
    
    // return to owner. It's ok to clobber the stack because it will
    // never be touched again
    ctx_swap(&self, *owner);
    assert(false);
}

void ctx_exit(void* rval) {
    ctx_exit(ctx_current(), rval);
}

// dirty hack: this function is only ever "called" by returning from
// ctx_swap for the first time a context is scheduled. Even though
// ctx_swap won't explicinly pass the params, it doesn't clobber the
// ones passed to it so we're ok.
static
void ctx_start() {
    // look up our context
    context_map::iterator it = ctx_current();
    ctx_context &ctx = it->second;
    
    // exit with function's rval
    try {
        ctx_exit(it, (ctx.func)(ctx.arg));
    } catch(...) {
        printf("Context terminated by uncaught exception!\n");
        exit(-1);
    }
}

extern ctx_handle* ctx_init(void* ctx_stack, void (*start)(void));

ctx_handle* ctx_create(void* (*func)(void*), void* arg,
                          ctx_handle** owner, size_t stack_size) {
    static int const K = 1024;
    static int const M = K*K;
    
    // Zero size is a hint by the caller to use a reaonable default
    if(stack_size == 0)
        stack_size = 1*M;

    void* buffer;
    if(0) {
        buffer = mmap(0, stack_size, 
                      PROT_READ|PROT_WRITE,
                      MAP_PRIVATE|MAP_ANONYMOUS|MAP_GROWSDOWN,
                      0, 0);
    }
    else {
        char* base = new char[stack_size];
        buffer = base + stack_size;
        VALGRIND_STACK_REGISTER(base, buffer);
    }
    // allocation failure?
    if(buffer == (void*)-1)
        EXCEPTION(BadAlloc);

    // register it for later
    ctx_context &ctx = ctx_live[buffer];
    ctx.stack_size = stack_size;
    ctx.func = func;
    ctx.arg = arg;
    ctx.rval = NULL;
    ctx.owner = owner;
    ctx.self = ctx_init(buffer, &ctx_start);
    
    // return handle to caller
    return ctx.self;
}

