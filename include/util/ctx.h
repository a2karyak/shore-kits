/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _CTX_H
#define _CTX_H

#include "util/c_str.h"
#include <ucontext.h>


struct ctx_t {

public:
    
    c_str      _context_name;
    ucontext_t _context;
  
    ctx_t(const c_str &name)
        : _context_name(name)
    {
        int getctx = getcontext(&_context);
        assert(getctx == 0);
    }
};


#endif
