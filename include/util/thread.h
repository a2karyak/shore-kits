/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __THREAD_H
#define __THREAD_H


// pthread.h should always be the first include!
#include <pthread.h>
#include <cstdio>
#ifdef __GCC
#include <cstdlib>
#else
#include <stdlib.h> // on Sun's CC <stdlib.h> defines rand_r, <cstdlib> doesn't
#endif
#include <cerrno>
#include <cassert>
#include <functional>
#include <cstdarg>
#include <stdint.h>
#include <time.h>
#include <ucontext.h>

#include "util/c_str.h"
#include "util/exception.h"
#include "util/ctx.h"


DEFINE_EXCEPTION(ThreadException);


#ifndef __GCC
//using std::rand_r;
#endif

pthread_mutex_t thread_mutex_create(const pthread_mutexattr_t* attr=NULL);
void thread_mutex_lock(pthread_mutex_t &mutex);
void thread_mutex_unlock(pthread_mutex_t &mutex);
void thread_mutex_destroy(pthread_mutex_t &mutex);


pthread_cond_t thread_cond_create(const pthread_condattr_t* attr=NULL);
void thread_cond_destroy(pthread_cond_t &cond);
void thread_cond_signal(pthread_cond_t &cond);
void thread_cond_broadcast(pthread_cond_t &cond);
void thread_cond_wait(pthread_cond_t &cond, pthread_mutex_t &mutex);
bool thread_cond_wait(pthread_cond_t &cond, pthread_mutex_t &mutex,
                           struct timespec &timeout);

template <class T>
T* thread_join(pthread_t tid) {
    // the union keeps gcc happy about the "type-punned" pointer
    // access going on here. Otherwise, -O3 could break the code.
    union {
        void *p;
        T *v;
    } u;
    if(pthread_join(tid, &u.p))
        THROW(ThreadException);

    return u.v;
}




/**
 *  @brief QPIPE thread base class. Basically a thin wrapper around an
 *  internal method and a thread name.
 */
class thread_t {

private:
    
    c_str        _thread_name;
    unsigned int _rand_seed;
    
protected:
    bool _delete_me;

public:

    ctx_t* _ctx;

    virtual void* run()=0;
    
    bool delete_me() { return _delete_me; }
    
    c_str thread_name() {
        return _thread_name;
    }


    /**
     *  @brief Returns a pseudo-random integer between 0 and RAND_MAX.
     */
    void reset_rand();


    /**
     *  @brief Returns a pseudo-random integer between 0 and RAND_MAX.
     */
    int rand() {
        return rand_r(&_rand_seed);
    }

    /**
     * Returns a pseudorandom, uniformly distributed int value between
     * 0 (inclusive) and the specified value (exclusive).
     *
     * Source http://java.sun.com/j2se/1.5.0/docs/api/java/util/Random.html#nextInt(int)
     */
    int rand(int n) {
        assert(n > 0);

        if ((n & -n) == n)  // i.e., n is a power of 2
            return (int)((n * (uint64_t)rand()) / (RAND_MAX+1));

        int bits, val;
        do {
            bits = rand();
            val = bits % n;
        } while(bits - val + (n-1) < 0);
        
        return val;
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
          _instance(instance),
          _func(func)
    {
    }
    
    virtual void* run() {
        return _func(_instance);
    }
};



/**
 *  @brief Helper function for running class member functions in a
 *  pthread. The function must take no arguments and return a type
 *  compatible with (void*).
 */
template <class Return, class Class>
thread_t* member_func_thread(Class *instance,
                             Return (Class::*mem_func)(),
                             const c_str& thread_name)
{
    typedef std::mem_fun_t<Return, Class> Functor;
    return new member_func_thread_t<Class, Functor>(instance,
                                                    Functor(mem_func),
                                                    thread_name);
}

struct thread_pool {
    pthread_mutex_t _lock;
    pthread_cond_t _cond;
    int _max_active;		// how many can be active at a time?
    int _active;		// how many are actually active?
    thread_pool(int max_active)
	: _lock(thread_mutex_create()),
	  _cond(thread_cond_create()),
	  _max_active(max_active), _active(0)
    {
    }

    void start();
    void stop();
};



// exported functions

void      thread_init(void);
thread_t* thread_get_self(void);
pthread_t thread_create(thread_t* t, thread_pool* p=NULL);

template<typename T>
struct thread_local {
    pthread_key_t _key;
    
    // NOTE:: the template is here because there because I am not
    // aware of a way to pass 'extern "C" function pointers to a C++
    // class member function. Fortunately the compiler can still
    // instantiate a template taking 'extern "C"' stuff... However,
    // this makes a default constructor argument of NULL ambiguous, so
    // we have to have two constructors instead. Yuck.
    template<class Destructor>
    thread_local(Destructor d) {
	pthread_key_create(&_key, d);
    }
    thread_local() {
	pthread_key_create(&_key, NULL);
    }
    ~thread_local() {
	pthread_key_delete(_key);
    }

    T* get() {
	return reinterpret_cast<T*>(pthread_getspecific(_key));
    }
    void set(T* val) {
	int result = pthread_setspecific(_key, val);
	THROW_IF(ThreadException, result);
    }
    

    thread_local &operator=(T* val) {
	set(val);
	return *this;
    }
    operator T*() {
	return get();
    }
};

extern thread_local<thread_pool> THREAD_POOL;


#endif  /* __THREAD_H */
