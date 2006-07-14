/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __C_STR_H
#define __C_STR_H


#include "engine/sync.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <cstdio>

#include <stdarg.h>
#include <new>
#include <cstring>

#define DEBUG_C_STR 0



class c_str {

    struct c_str_data {
        mutable pthread_mutex_t _lock;
        mutable unsigned _count;
        char _str[0];
    };

    static c_str_data _seed;

    c_str_data* _data;


    /**
     *  Decrement reference count, freeing memory if count hits 0.
     */
    void release() {
        // * * * BEGIN CRITICAL SECTION * * *
        critical_section_t cs(&_data->_lock);
        if(--_data->_count == 0) {
            if(DEBUG_C_STR)
                printf("Freeing %s\n", &_data->_str[0]);
            free(_data);
        }
        // * * * END CRITICAL SECTION * * *
    }    
    
    
    /**
     *  @brief Just increment reference count of other. We are
     *  guaranteed that other will remain allocated until we return
     *  since other contributes exactly 1 to the total reference
     *  count.
     */
    void assign(const c_str &other) {

        // other._data won't disappear (see above)
        _data = other._data;

        // * * * BEGIN CRITICAL SECTION * * *
        critical_section_t cs(&_data->_lock);
        _data->_count++;
        // * * * END CRITICAL SECTION * * *
    }
    

public:

    c_str (const c_str &other) {
        if (DEBUG_C_STR)
            printf("copy constructor with other = %s\n", other.data());
        assign(other);
    }
    
    
    c_str(const char* str, ...) {

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
            throw std::bad_alloc();
        
        va_end(args);

        // Copy in initial header value. Let's hope that vasprintf()'s
        // malloc() call return something 
        *pun.data = _seed;

        // done!
        _data = pun.data;
        
        if (DEBUG_C_STR)
            printf("constructed new c_str = %s\n", data());
    }


    const char* data() const {
        // Return the actual string... whatever uses this string must
        // copy it since it does not own it.
        return _data->_str;
    }
    
    
    ~c_str() {
        if (DEBUG_C_STR)
            printf("in c_str destructor for %s\n", data());
        release();
    }
    
    
    c_str &operator=(const c_str &other) {
        if (DEBUG_C_STR)
            printf("in c_str = operator, this = %s, other = %s\n", data(), other.data());
        release();
        assign(other);
        return *this;
    }
    
};



#endif
