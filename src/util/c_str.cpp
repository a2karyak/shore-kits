// -*- mode:c++; c-basic-offset:4 -*-

#include <stdarg.h>

#include "util/c_str.h"
#include "util/exception.h"
#include "util/sync.h"
#include "util/guard.h"
#include <stdio.h>

// Ignore the warning: printf("") is valid!
const c_str c_str::EMPTY_STRING("%s", "");

struct c_str::c_str_data {
    mutable pthread_mutex_t _lock;
    mutable unsigned _count;
    char* _str() { return sizeof(c_str_data) + (char*) this; }
};

c_str::c_str(const char* str, ...)
    : _data(NULL)
{

    for(int i=128; i <= 1024; i *= 2) {
        va_list args;
        array_guard_t<char> tmp = new char[i+1];
        va_start(args, str);
        int count = vsnprintf(tmp, i, str, args);
        va_end(args);
        
        if(count > i)
            continue;
        
        int len = strlen(tmp);
        _data = (c_str_data*) malloc(sizeof(c_str_data) + len);
        if(_data == NULL)
            THROW(BadAlloc);
        
        _data->_lock = thread_mutex_create();
        _data->_count = 1;
        memcpy(_data->_str(), tmp, len+1);
        return;
    }
    // shouldn't get here...
    THROW(BadAlloc);
}

const char* c_str::data() const {
    // Return the actual string... whatever uses this string must
    // copy it since it does not own it.
    return _data->_str();
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
            printf("Freeing %s\n", _data->_str());
        free(_data);
    }
    // * * * END CRITICAL SECTION * * *
}    
    
    
