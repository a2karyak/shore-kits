/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
      Data Intensive Applications and Systems Labaratory (DIAS)
               Ecole Polytechnique Federale de Lausanne
   
                         All Rights Reserved.
   
   Permission to use, copy, modify and distribute this software and
   its documentation is hereby granted, provided that both the
   copyright notice and this permission notice appear in all copies of
   the software, derivative works or modified versions, and any
   portions thereof, and that both notices appear in supporting
   documentation.
   
   This code is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
   DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
   RESULTING FROM THE USE OF THIS SOFTWARE.
*/

/** @file:   cache.h
 *
 *  @brief:  Template-based cache objects
 *
 *  @author: Ippokratis Pandis, Dec 2008
 */

#ifndef __UTIL_OBJECT_CACHE_H
#define __UTIL_OBJECT_CACHE_H

#include "k_defines.h"

#include "atomic_trash_stack.h"
#include "util/guard.h"
#include "util/stl_pooled_alloc.h"


// define cache stats to get some statistics about the tuple cache
#undef CACHE_STATS
//#define CACHE_STATS


const int DEFAULT_INIT_OBJECT_COUNT = 10;
//const int DEFAULT_INIT_OBJECT_COUNT = 100;



/******************************************************************** 
 *
 * @class: cacheable_iface
 *
 * @brief: Interface for objects that can be used by the object cache
 *
 * @note:  Any class that we want to use the cache needs to implement
 *         this interface
 * 
 ********************************************************************/

class cacheable_iface
{
public:
    virtual ~cacheable_iface() {}

    // INTERFACE

    virtual void setup(Pool** stl_pool_list)=0;
    virtual void reset()=0;

}; // EOF: cacheable_iface




/******************************************************************** 
 *
 * @class: object_cache_t
 *
 * @brief: (template-based) object cache of cacheable objects
 *
 * @note:  The Object needs to implement the cacheable_iface
 * 
 ********************************************************************/
template <typename T>
class object_cache_t;

template <typename T>
void* operator new(size_t nbytes, object_cache_t<T>& cache) ;
template <typename T>
void operator delete(void* ptr, object_cache_t<T>& cache) ;

template <typename Object>
class object_cache_t : protected atomic_stack
{
private:
    Pool** _stl_pools;
    const int _nbytes;
   
#ifdef CACHE_STATS    
    // stats
    int _object_requests; 
    int _object_setups;
#endif              

    // start with a non-empty pool, so that threads don't race
    // at the beginning
    struct mylink {
        Object* _pobj;
        mylink* _next;
        mylink() : _pobj(NULL), _next(NULL) { }
        mylink(Object* obj, mylink* next) : _pobj(obj), _next(next) { }
        ~mylink() { }
    };        

    
    Object* _do_alloc() 
    {
        vpn u = { malloc(_nbytes) };
        if (!u.v) u.v = null();
        Object* pobj = (Object*)prepare(u);
	return (pobj);
    }

public:

    // these guys need to access the underlying object cache
    friend void* operator new<>(size_t, object_cache_t<Object>&);
    friend void operator delete<>(void*, object_cache_t<Object>&);


    object_cache_t(Pool** stlpools, int init_count = DEFAULT_INIT_OBJECT_COUNT) 
        : atomic_stack(-sizeof(ptr)), _stl_pools(stlpools), _nbytes(sizeof(Object)+sizeof(ptr))
#ifdef CACHE_STATS
        , _object_requests(0), _object_setups(0)
#endif
    { 
        assert (stlpools);
        assert (init_count); 
        mylink head;
        mylink* node = NULL;

#ifdef CFG_CACHES
        // create (init_count) objects, and push them to the cache
        for (int i=0; i<init_count; i++) {
            Object* u = borrow();
            node = new mylink(u,head._next);
            head._next = node;
        }

        for (int i=0; i<init_count; i++) {
            giveback(node->_pobj);
            head._next = node;
            node = node->_next;
            delete (head._next);
        }
#endif
    }

    
    // Destroys the cache, calling the destructor for all the objects 
    // it is hoarding.
    virtual ~object_cache_t() 
    {
        vpn val;
        void* v = NULL;
        int icount = 0;
        while ( (v=pop()) ) {
            val.v = v;
            val.n += _offset; // back up to the real
            ((Object*)v)->~Object();
            free(val.v);
            ++icount;
        }
        //printf("Deleted: (%d)\n", icount);
    }


    // Ask for an unused object, if cache empty allocate and return a new one
    Object* borrow() 
    {
#ifdef CFG_CACHES
#ifdef CACHE_STATS
        CRITICAL_SECTION( rcs, _requests_lock);
        ++_object_requests;
#endif

        void* val = pop();
        if (val) return ((Object*)(val));

#ifdef CACHE_STATS
        CRITICAL_SECTION( scs, _setups_lock);
        ++_object_setups;
#endif

        Object* temp = new (*this) Object();
        temp->setup(_stl_pools);
        return (temp);
#else
        // Calls malloc, should used only for debugging purposes
        Object* temp = new Object();
        temp->setup(_stl_pools);
        return (temp);
#endif
    }
    

    // Returns an object to the cache. The object is reset and put on the
    // free list.
    void giveback(Object* pobj) 
    {        
#ifdef CFG_CACHES
        assert (pobj);
        
        // avoid pointer aliasing problems with the optimizer
        union { Object* t; void* v; } u = {pobj};

        // reset the object
        u.t->reset();
        
        // give it back
        push(u.v);
#else
        delete (pobj);
#endif
    }    


#ifdef CACHE_STATS    
    int  setup_count() { return (_object_setups); }
    int  request_count() { return (_object_requests); }
    void print_stats() {
        TRACE( TRACE_STATISTICS, "Requests: (%d)\n", _object_requests);
        TRACE( TRACE_STATISTICS, "Setups  : (%d)\n", _object_setups);
    }
#endif 

}; // EOF: object_cache_t



/* Usage: Object* pobj = new (object_cache_t<Object>) Object(...)
   when finished, call object_cache_t<Object>::destroy(pobj) instead of delete.
 */

template <typename T>
inline void* operator new(size_t nbytes, object_cache_t<T>& cache) 
{
    assert(cache._nbytes >= nbytes);
    return (cache._do_alloc());
}


/* Called automatically by the compiler if T's constructor throws
   (otherwise memory would leak).
   Unfortunately, there is no "delete (cache)" syntax in C++ so the user
   must still call cache::destroy()
 */
template <typename T>
inline void operator delete(void* ptr, object_cache_t<T>& cache) 
{
    cache.giveback((T*)ptr);
}



#endif /* __UTIL_OBJECT_CACHE_H */
