/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __C_STR_H
#define __C_STR_H


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <cstdio>
#include <cstring>

#define DEBUG_C_STR 0



class c_str {

    struct c_str_data;
    c_str_data* _data;


    /**
     *  Decrement reference count, freeing memory if count hits 0.
     */
    void release();    
    
    /**
     *  @brief Just increment reference count of other. We are
     *  guaranteed that other will remain allocated until we return
     *  since other contributes exactly 1 to the total reference
     *  count.
     */
    void assign(const c_str &other);    

public:
    static const c_str EMPTY_STRING;

    c_str (const c_str &other=EMPTY_STRING) {
        if (DEBUG_C_STR)
            printf("copy constructor with other = %s\n", other.data());
        assign(other);
    }
    
    // start counting params at 2 instead of 1 -- non-static member function
    c_str(const char* str, ...) __attribute__((format(printf, 2, 3)));
    
    operator const char*() const {
        return data();
    }

    const char* data() const;
    
    
    ~c_str() {
        if (DEBUG_C_STR)
            printf("in c_str destructor for %s\n", data());
        release();
    }
    
    
    c_str &operator=(const c_str &other) {
        if (DEBUG_C_STR)
            printf("in c_str = operator, this = %s, other = %s\n", data(), other.data());
        if(_data != other._data) {
            release();
            assign(other);
        }
        return *this;
    }
    

    bool operator<(const c_str &other) const {
        return strcmp(data(), other.data()) < 0;
    }


    bool operator>(const c_str &other) const {
        return strcmp(data(), other.data()) > 0;
    }

    
    bool operator==(const c_str &other) const {
        return strcmp(data(), other.data()) == 0;
    }
};



#endif
