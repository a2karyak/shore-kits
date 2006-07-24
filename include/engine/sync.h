/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _SYNC_H
#define _SYNC_H


// pthread.h should always be the first include!
#include <pthread.h>
#include "trace.h"
#include "qpipe_panic.h"



// exported functions

void pthread_mutex_init_wrapper(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr);
void pthread_mutex_lock_wrapper(pthread_mutex_t* mutex);
void pthread_mutex_unlock_wrapper(pthread_mutex_t* mutex);
void pthread_mutex_destroy_wrapper(pthread_mutex_t* mutex);


void pthread_cond_init_wrapper(pthread_cond_t* cond, const pthread_condattr_t* attr);
void pthread_cond_destroy_wrapper(pthread_cond_t* cond);
void pthread_cond_signal_wrapper(pthread_cond_t* cond);
void pthread_cond_broadcast_wrapper(pthread_cond_t* cond);
void pthread_cond_wait_wrapper(pthread_cond_t* cond, pthread_mutex_t* mutex);



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
 *  @brief A critical section manager. Locks and unlocks the specified
 *  mutex upon construction and destruction respectively.
 */

struct critical_section_t {
    pthread_mutex_t* _mutex;
    critical_section_t(pthread_mutex_t* mutex)
        : _mutex(mutex)
    {
        pthread_mutex_lock_wrapper(_mutex);
    }
    void exit() {
        if(_mutex) {
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



#endif
