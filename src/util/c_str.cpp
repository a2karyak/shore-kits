/* -*- mode:C++; c-basic-offset:4 -*- */

#include <stdarg.h>

#include "util/c_str.h"
#include "util/exception.h"
#include "util/sync.h"
#include "util/guard.h"
#include <stdio.h>

#ifdef __SUNPRO_CC
#include <atomic.h>
#endif

#include "util/pool_alloc.h"

// Ignore the warning: printf("") is valid!
const c_str c_str::EMPTY_STRING("%s", "");

static pool_alloc c_str_alloc;

struct c_str::c_str_data {
#ifndef __SUNPRO_CC
    // Sun provides atomic_add primitives we can use instead of locks
    mutable pthread_mutex_t _lock;
#endif
    mutable unsigned _count;
    char* _str() { return sizeof(c_str_data) + (char*) this; }
};



c_str::c_str(const char* str, ...)
    : _data(NULL)
{

    static int const i = 1024;
    char tmp[i];
	
    va_list args;
    va_start(args, str);
    int count = vsnprintf(tmp, i, str, args);
    va_end(args);
        
    if((count < 0) /* glibc 2.0 */ || (count >= i) /* glibc 2.1 */)
	THROW(BadAlloc); // oops!

    
    _data = (c_str_data*) c_str_alloc.alloc(sizeof(c_str_data) + count + 1);
    if(_data == NULL)
	THROW(BadAlloc);

#ifndef __SUNPRO_CC
    _data->_lock = thread_mutex_create();
#endif
    _data->_count = 1;
    memcpy(_data->_str(), tmp, count+1);
}



const char* c_str::data() const {
    // Return the actual string... whatever uses this string must
    // copy it since it does not own it.
    return _data->_str();
}



void c_str::assign(const c_str &other) {

    // other._data won't disappear because it can't be destroyed before we return.
    _data = other._data;

#ifdef __SUNPRO_CC
    atomic_add_32(&_data->_count, 1);
#else
    // * * * BEGIN CRITICAL SECTION * * *
    critical_section_t cs(_data->_lock);
    _data->_count++;
    // * * * END CRITICAL SECTION * * *
#endif
}



void c_str::release() {
    int count;
#ifdef __SUNPRO_CC
    count = atomic_add_32_nv(&_data->_count, -1);
#else
    // * * * BEGIN CRITICAL SECTION * * *
    {
	critical_section_t cs(_data->_lock);
	count = --_data->_count;
    }
    // * * * END CRITICAL SECTION * * *
#endif
    if(count == 0) {
        // we were the last reference, so nobody else could
        // possibly modify the data struct any more
            
        if(DEBUG_C_STR)
            printf("Freeing %s\n", _data->_str());
        c_str_alloc.free(_data);
    }
}
