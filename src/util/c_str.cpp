// -*- mode:c++; c-basic-offset:4 -*-

#include <stdarg.h>

#include "util/c_str.h"
#include "util/exception.h"
#include "util/sync.h"

// Ignore the warning: printf("") is valid!
const c_str c_str::EMPTY_STRING = "";

struct c_str::c_str_data {
    mutable pthread_mutex_t _lock;
    mutable unsigned _count;
    char _str[0] __attribute__ ((aligned));
};

c_str::c_str(const char* str, ...) {

    int size = strlen(str) + 1;
    char format[sizeof(c_str_data) + size];

    // pad with space for the header, but don't copy it in quite yet
    memset(format, '.', sizeof(c_str_data));
    // copy over the format string
    memcpy(&format[sizeof(c_str_data)], str, size);

    // keep the compiler's memory analysis happy (type-punned access!)
    union {
        char* str;
        c_str_data* data;
    } pun;


    // copy the string
    va_list args;
    va_start(args, str);
        
    // format the string
    if( vasprintf(&pun.str, format, args) < 0)
        throw EXCEPTION(BadAlloc);
        
    va_end(args);

    // Copy in initial header value. Let's hope that vasprintf()'s
    // malloc() call return something 
    pun.data->_lock = thread_mutex_create();
    pun.data->_count = 1;

    // done!
    _data = pun.data;
        
    if (DEBUG_C_STR)
        printf("constructed new c_str = %s\n", data());
}

const char* c_str::data() const {
    // Return the actual string... whatever uses this string must
    // copy it since it does not own it.
    return _data->_str;
}

void c_str::assign(const c_str &other) {

    // other._data won't disappear (see above)
    _data = other._data;

    // * * * BEGIN CRITICAL SECTION * * *
    critical_section_t cs(_data->_lock);
    _data->_count++;
    // * * * END CRITICAL SECTION * * *
}

void c_str::release() {
    // * * * BEGIN CRITICAL SECTION * * *
    critical_section_t cs(_data->_lock);
    if(--_data->_count == 0) {
        // we were the last reference, so nobody else could
        // possibly modify the data struct any more
        cs.exit();
            
        if(DEBUG_C_STR)
            printf("Freeing %s\n", &_data->_str[0]);
        free(_data);
    }
    // * * * END CRITICAL SECTION * * *
}    
    
    
