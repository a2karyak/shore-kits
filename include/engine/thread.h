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



/**
 *  @brief QPIPE thread base class. Basically a thin wrapper around an
 *  internal method and a thread name.
 */
class thread_t {

private:

    char* thread_name;

public:

    thread_t(const char* format, ...);
    virtual ~thread_t();


    virtual void* run() {
        // should always be over-ridden, but never called for the root
        // thread
        assert(false);
    }

    const char* get_thread_name();
    
protected:

    thread_t(void);

    int init_thread_name  (const char* format, ...);
    int init_thread_name_v(const char* format, va_list ap);
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
    member_func_thread_t(Class *instance, Functor func,
                         const char *format, va_list args)
        : _instance(instance), _func(func)
    {
        init_thread_name_v(format, args);
    }
    
    virtual void *run() {
        return _func(_instance);
    }
};



// exported functions

void      thread_init(void);
thread_t* thread_get_self(void);
int       thread_create(pthread_t* thread, thread_t* t);

void pthread_mutex_init_wrapper(pthread_mutex_t* mutex,
				const pthread_mutexattr_t* attr);
void pthread_mutex_lock_wrapper(pthread_mutex_t* mutex);
void pthread_mutex_unlock_wrapper(pthread_mutex_t* mutex);
void pthread_mutex_destroy_wrapper(pthread_mutex_t* mutex);


void pthread_cond_init_wrapper(pthread_cond_t* cond,
			       const pthread_condattr_t* attr);
void pthread_cond_destroy_wrapper(pthread_cond_t* cond);
void pthread_cond_signal_wrapper(pthread_cond_t* cond);
void pthread_cond_wait_wrapper(pthread_cond_t* cond,
			       pthread_mutex_t* mutex);

template <class T>
void pthread_join_wrapper(pthread_t tid, T* &rval) {
    // the union keeps gcc happy about the "type-punned" pointer
    // access going on here. Otherwise, -O3 could break the code.
    union {
        void *p;
        T *v;
    } u;
    if(pthread_join(tid, &u.p)) {
        TRACE(TRACE_ALWAYS, "pthread_join() failed");
        QPIPE_PANIC();
    }

    rval = u.v;
}



/**
 * @brief Helper function for running class member functions in a
 * pthread. The function must take no arguments and return a type
 * compatible with (void*).
 */
template <class Return, class Class>
thread_t *member_func_thread(Class *instance, Return (Class::*mem_func)(),
                             const char *format, ...)
{
    typedef std::mem_fun_t<Return, Class> Functor;
    va_list args;
    member_func_thread_t<Class, Functor> *thread;
    va_start(args, format);
    thread = new member_func_thread_t<Class, Functor>(instance,
                                                      Functor(mem_func),
                                                      format, args);
    va_end(args);
    return thread;
}



/**
 *  @brief A critical section manager. Locks and unlocks the specified
 *  mutex upon construction and destruction respectively.
 */

struct critical_section_t {
    pthread_mutex_t *_mutex;
    critical_section_t(pthread_mutex_t *mutex)
        : _mutex(mutex)
    {
        pthread_mutex_lock_wrapper(_mutex);
        //        printf("Locked mutex %p\n", _mutex);
    }
    void exit() {
        if(_mutex) {
            //            printf("Unlocked mutex %p\n", _mutex);
            pthread_mutex_unlock_wrapper(_mutex);
            _mutex = NULL;
        }
    }
    ~critical_section_t() {
        exit();
    }
};



/**
 * @brief A simple "notifier" that allows a thread to block on pending
 * events. Multiple notifications that arrive with no waiting thread
 * will be treated as one event.
 */
struct notify_t {
    volatile bool _notified;
    volatile bool _cancelled;
    pthread_mutex_t _lock;
    pthread_cond_t _notify;

    notify_t() {
        _notified = _cancelled = false;
        pthread_mutex_init_wrapper(&_lock, NULL);
        pthread_cond_init_wrapper(&_notify, NULL);
    }

    /**
     * @brief Blocks the calling thread until either notify() or
     * cancel() is called. If either method was called before wait()
     * return immediately.
     *
     * @return zero if notified, non-zero if cancelled.
     */
    int wait() {
        critical_section_t cs(&_lock);
        return wait_holding_lock();
    }

    /**
     * @brief Similar to wait(), except the caller is assumed to
     * already hold the lock for other reasons.
     */
    int wait_holding_lock() {
        while(!_notified && !_cancelled)
            pthread_cond_wait_wrapper(&_notify, &_lock);

        bool result = _cancelled;
        _notified = _cancelled = false;
        return result;
    }
    
    /**
     * @brief Wake up a waiting thread. If no thread is waiting the
     * event will be remembered. 
     */
    void notify() {
        critical_section_t cs(&_lock);
        notify_holding_lock();
    }

    /**
     * @brief Wake up a waiting thread from within an existing
     * critical section.
     *
     * WARNING: the caller MUST hold (_lock) or the behavior of this
     * function is undefined!
     */
    void notify_holding_lock() {
        signal(_notified);
    }

    void cancel() {
        critical_section_t cs(&_lock);
        cancel_holding_lock();
    }

    void cancel_holding_lock() {
        signal(_cancelled);
    }
    
protected:

    void signal(volatile bool &val) {
        val = true;
        pthread_cond_signal_wrapper(&_notify);
    }
};



#endif  /* __THREAD_H */
