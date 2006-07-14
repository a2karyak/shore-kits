// -*- mode:c++; c-basic-offset:4
#ifndef __C_STR_H
#define __C_STR_H

#include <stdarg.h>
#include <new>

class c_str {
    char* _str;
    
public:
    // creates a copy of the input string that is safe to manage with
    // a c_str (avoids delete/free mixups)
    static char* copy(const char* str) {
        return asprintf("%s", str);
    }
    // wraps up a call to asprintf to throw std::bad_alloc on error
    static char* asprintf(const char* format, ...)
        __attribute__((format(printf, 1, 2)))
    {
        va_list args;
        va_start(args, format);
        char* result = vasprintf(format, args);
        va_end(args);
        return result;
    }
    // wraps up a call to vasprintf to throw std::bad_alloc on error
    static char* vasprintf(const char* format, va_list args) {
        char* result;
        if(::vasprintf(&result, format, args) < 0)
            throw std::bad_alloc();
        
        return result;
    }
    
    c_str(const char* str) {
        assert(str);
        _str = copy(str);
    }
    
    ~c_str() {
        free(_str);
    }

    operator const char*() const {
        return _str;
    }
    const char* data() const {
        return _str;
    }
    
    char* clone() const {
        return copy(_str);
    }
    
    // force deep copy
    c_str(const c_str &other)
        : _str(other.clone())
    {
    }
    
    c_str &operator=(const c_str &other) {
        free(_str);
        _str = other.clone();
        return *this;
    }
};


#endif
