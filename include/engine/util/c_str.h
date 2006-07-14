// -*- mode:c++; c-basic-offset:4 -*-
#ifndef __C_STR_H
#define __C_STR_H

#include <stdarg.h>
#include <new>

#if 1
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
#else
class c_str {
    static c_str_data _seed;
    
    struct c_str_data {
        pthread_mutex_t _lock;
        unsigned _count;
        char _str[0];
    };

    c_str_data* _data;
    
 public:
    c_str(const char* str, ...) {
        // copy the string
        va_list args;
        va_start(args, format);

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

        // format the string
        if(vasprintf(&pun.str, format, args) < 0)
            throw std::bad_alloc();
        
        va_end(args);

        // copy the header in
        *pun.data = _seed;

        // done!
        _data = pun.data;
    }

    const char* data() {
        // skip to the actual string
        return _data->_str;
    }
    ~c_str() {
        if(--_data->count == 0)
            free(_data);
    }
private:
};
#endif

#endif
