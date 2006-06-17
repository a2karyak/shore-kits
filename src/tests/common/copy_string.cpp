/* -*- mode:C++; c-basic-offset:4 -*- */

#include "tests/common/copy_string.h"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <cstdio>
#include <cstring>
#include <cassert>



char* copy_string(const char* format, ...) {
    
    va_list ap;
    va_start(ap, format);
    
    // construct string
    char* str;
    int ret = vasprintf(&str, format, ap);
    assert(ret != -1);

    va_end(ap);
    return str;
}
