/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _GUARD_H
#define _GUARD_H

#include <cstdio>
#include <cassert>
#include <sys/time.h>
#include "trace.h"
#include "c_str.h"
#include <unistd.h>


/**
 * @brief A generic pointer guard base class. An action will be
 * performed exactly once on the stored pointer, either with an
 * explicit call to done() or when the guard goes out of scope,
 * whichever comes first.
 *
 * This class is much like the auto_ptr class, other than allowing
 * actions besides delete upon destruct. In particular it is *NOT
 * SAFE* to use it in STL containers because it does not fulfill the
 * Assignable concept.
 *
 * This class *DOES NOT* support being cast as "const" in the
 * traditional sense. Other than disallowing use of the assignment
 * operator '=' it always allows modification of its contents so that
 * it do its job properly. This should not be a problem in practice
 * because a "const T" (with T = <pointer to some type>, eg int*) does
 * not actually protect a pointer's contents from modification
 * anyway. Use T = <pointer to some const type> for that, eg const
 * int*.
 *
 */
template <class T, class Impl>
class guard_base_t {
    mutable T _obj;

public:
    typedef guard_base_t<T, Impl> Base;

    // The value that represents "null" for typename T. Impl usually
    // descends from this class so this makes a handy default
    // implmentation
    static T null_value() {
        return NULL;
    }
    
    guard_base_t(T obj)
        : _obj(obj)
    {
    }
    guard_base_t(const guard_base_t &other)
        : _obj(other.release())
    {
    }
    guard_base_t &operator =(const guard_base_t &other) {
        assign(other.release());
        return *this;
    }
    
    operator T() const {
        return get();
    }

    T get() const {
        return _obj;
    }
    
    /**
     * @brief Notifies this guard that its services are no longer
     * needed because some other entity has assumed ownership of the
     * pointer.
     *
     * NOTE: this function is marked const so that the assignment
     * operator and copy constructor can work properly. 
     */
    T release() const {
        T obj = _obj;
        _obj = Impl::null_value();
        return obj;
    }

    /**
     * @brief Notifies this guard that its action should be performed
     * now rather than at destruct time.
     */
    void done() const {
        assign(Impl::null_value());
    }
    
    
    ~guard_base_t() {
        done();
    }
    
protected:
    void assign(T obj) const {
        if(_obj == obj)
            return;
        
        // free the old object and set it NULL
        Impl::guard_action(_obj);
        _obj = obj;
    }
    
};

template <typename T, typename Impl>
struct pointer_guard_base_t : guard_base_t<T*, Impl> {
    typedef guard_base_t<T*, Impl> Base;
    pointer_guard_base_t(T* ptr)
        : Base(ptr)
    {
    }
    T &operator *() const {
        return *Base::get();
    }
    operator T*() const {
        return Base::get();
    }
    T* operator ->() const {
        return Base::get();
    }
};


/**
 * @brief Ensures that a pointer allocated with a call to operator
 * new() gets deleted
 */
template <class T>
struct pointer_guard_t : pointer_guard_base_t<T, pointer_guard_t<T> > {
    pointer_guard_t(T* ptr=NULL)
        : pointer_guard_base_t<T, pointer_guard_t<T> >(ptr)
    {
    }
    static void guard_action(T* ptr) {
        delete ptr;
    }
};



/**
 * @brief Ensures that an array allocated with a call to operator
 * new() gets deleted
 */
template <class T>
struct array_guard_t : pointer_guard_base_t<T, array_guard_t<T> > {
    array_guard_t(T *ptr=NULL)
        : pointer_guard_base_t<T, array_guard_t<T> >(ptr)
    {
    }
    static void guard_action(T *ptr) {
        delete [] ptr;
    }
};



/**
 * @brief Ensures that a file gets closed
 */
struct file_guard_t : pointer_guard_base_t<FILE, file_guard_t> {
    file_guard_t(FILE *ptr=NULL)
        : pointer_guard_base_t<FILE, file_guard_t>(ptr)
    {
    }
    static void guard_action(FILE *ptr) {
        if(ptr && fclose(ptr)) 
            TRACE(TRACE_ALWAYS, "fclose failed");
    }
};

/**
 * @brief Ensures that a file descriptor gets closed
 */
struct fd_guard_t : guard_base_t<int, fd_guard_t> {
    fd_guard_t(int fd)
        : guard_base_t<int, fd_guard_t>(fd)
    {
    }
    static void guard_action(int fd) {
        if(fd >= 0 && close(fd)) 
            TRACE(TRACE_ALWAYS, "close() failed");
    }
    static int null_value() {
        return -1;
    }
};



#endif
