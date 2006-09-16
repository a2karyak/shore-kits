/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _SYNC_H
#define _SYNC_H

#include "util/thread.h"

// exported functions

/**
 *  @brief A critical section manager. Locks and unlocks the specified
 *  mutex upon construction and destruction respectively.
 */

struct critical_section_t {
    pthread_mutex_t* _mutex;
    critical_section_t(pthread_mutex_t &mutex)
        : _mutex(&mutex)
    {
        thread_mutex_lock(*_mutex);
    }
    
    void enter(pthread_mutex_t &mutex) {
        exit();
        _mutex = &mutex;
        thread_mutex_lock(*_mutex);
    }
    void exit() {
        if(_mutex) {
            thread_mutex_unlock(*_mutex);
            _mutex = NULL;
        }
    }
    ~critical_section_t() {
        exit();
    }
private:
    critical_section_t(critical_section_t const &);
    critical_section_t &operator =(critical_section_t const &);
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

    notify_t()
        : _notified(false),
          _cancelled(false),
          _lock(thread_mutex_create()),
          _notify(thread_cond_create())
    {
    }

    /**
     * @brief Blocks the calling thread until either notify() or
     * cancel() is called. If either method was called before wait()
     * return immediately.
     *
     * @return zero if notified, non-zero if cancelled.
     */
    int wait() {
        critical_section_t cs(_lock);
        return wait_holding_lock();
    }

    /**
     * @brief Similar to wait(), except the caller is assumed to
     * already hold the lock for other reasons.
     */
    int wait_holding_lock() {
        while(!_notified && !_cancelled)
            thread_cond_wait(_notify, _lock);

        bool result = _cancelled;
        _notified = _cancelled = false;
        return result;
    }
    
    /**
     * @brief Wake up a waiting thread. If no thread is waiting the
     * event will be remembered. 
     */
    void notify() {
        critical_section_t cs(_lock);
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
        critical_section_t cs(_lock);
        cancel_holding_lock();
    }

    void cancel_holding_lock() {
        signal(_cancelled);
    }
    
protected:

    void signal(volatile bool &val) {
        val = true;
        thread_cond_signal(_notify);
    }
};



#endif
