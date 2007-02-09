/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _CTX_H
#define _CTX_H

#include "util/c_str.h"
#include <ucontext.h>
#include <cstdio>


/** @brief Wrapper around a ucontext_t. */
class ctx_t {

public:
    
    c_str      _context_name;
    ucontext_t _context;

    ctx_t(const c_str &name)
        : _context_name(name)
    {
        int getctx = getcontext(&_context);
        assert(getctx == 0);
    }

    virtual ~ctx_t() { }
};


/** @brief A user-level context for which we need to allocate a
 *  stack. */
class worker_ctx_t : public ctx_t {

private:

    void* _stack;
    
#define CTX_KB 1024
#define CTX_MB ((CTX_KB) * (CTX_KB))
    
public:
    worker_ctx_t(const c_str &name, struct ucontext* uc_link)
        : ctx_t(name)
    {
        unsigned long stack_size = 500 * CTX_MB;

        /* Allocate a new stack */
        void* stack = malloc(stack_size);
        assert(stack != NULL); /* TODO change this to throw an exception */

        /* Initialize context with this stack */
        _context.uc_stack.ss_sp    = stack;
        _context.uc_stack.ss_flags = 0;
        _context.uc_stack.ss_size  = stack_size;
        _context.uc_link           = uc_link;

        printf("Created worker_ctx %s\n", _context_name.data());
        fflush(stdout);
    }


    ~worker_ctx_t() {
        printf("Destroying worker_ctx %s\n", _context_name.data());
        fflush(stdout);
        free(_context.uc_stack.ss_sp);
        _context.uc_stack.ss_sp = NULL;
    }
};


#endif
