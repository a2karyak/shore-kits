/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __THREAD_H
#define __THREAD_H


// pthread.h should always be the first include!
#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cassert>
#include <functional>
#include <cstdarg>

#include "qpipe_panic.h"
#include "trace.h"
#include "util/guard.h"
#include "engine/sync.h"
#include "engine/util/c_str.h"



/**
 *  @brief QPIPE thread base class. Basically a thin wrapper around an
 *  internal method and a thread name.
 */
class thread_t {

private:

    c_str thread_name;
    unsigned int rand_seed;

public:

    virtual void* run() {
        // should always be over-ridden, but never called for the root
        // thread
        assert(false);
    }


    c_str get_thread_name() {
        return thread_name;
    }


    int get_rand() {
        return rand_r(&rand_seed);
    }

    virtual ~thread_t() { }
    
protected:
    
    thread_t(const c_str &name);
        
};



/**
 *  @brief wraps up a class instance and a member function to look
 *  like a thread_t. Use the convenience function below to
 *  instantiate.
 */
template <class Class, class Functor>
class member_func_thread_t : public thread_t {
    Class *_instance;
    Functor _func;
public:
    member_func_thread_t(Class *instance, Functor func, const c_str &thread_name)
        : thread_t(thread_name),
          _instance(instance), _func(func)
    {
    }
    
    virtual void *run() {
        return _func(_instance);
    }
};



/**
 *  @brief Helper function for running class member functions in a
 *  pthread. The function must take no arguments and return a type
 *  compatible with (void*).
 */
template <class Return, class Class>
thread_t* member_func_thread(Class *instance, Return (Class::*mem_func)(), const c_str& thread_name)
{
    typedef std::mem_fun_t<Return, Class> Functor;
    return
        new member_func_thread_t<Class, Functor>(instance, Functor(mem_func), thread_name);
}



// exported functions

void      thread_init(void);
thread_t* thread_get_self(void);
int       thread_create(pthread_t* thread, thread_t* t);



#endif  /* __THREAD_H */
