/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   cache.h
 *
 *  @brief:  Template-based cache for DORA objects
 *
 *  @author: Ippokratis Pandis, Nov 2008
 */

#ifndef __DORA_OBJECT_CACHE_H
#define __DORA_OBJECT_CACHE_H


// sparcv9 only at the moment
#include <atomic.h>

#include "util/guard.h"
#include "util/stl_pooled_alloc.h"


// define cache stats to get some statistics about the tuple cache
#undef CACHE_STATS
//#define CACHE_STATS

ENTER_NAMESPACE(dora);


const int DEFAULT_INIT_OBJECT_COUNT = 100;



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

struct cacheable_iface
{
    cacheable_iface() {}
    ~cacheable_iface() {}

    // INTERFACE

    virtual void setup(Pool** stl_pool_list);
    virtual void reset();

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

template <typename Object>
class object_cache_t : protected atomic_stack
{
protected:
    Pool** _stl_pools;
   
#ifdef CACHE_STATS    
private:
    // stats
    int _object_requests; 
    tatas_lock _object_lock;
    int _object_setups;
    tatas_lock _object_lock;
#endif              

public:

    object_cache_t(Pool** stlpools = NULL, int init_count = DEFAULT_INIT_OBJECT_COUNT) 
        : atomic_stack(-sizeof(ptr)), _stl_pools(stlpools)
#ifdef CACHE_STATS
        , _object_requests(0), _object_setups(0)
#endif
    { 
        assert (init_count); 

        // start with a non-empty pool, so that threads don't race
        // at the beginning
        ptr* head = NULL;

        // create (init_count) objects, and push them to the cache
        for (int i=0; i<init_count; i++) {
            vpn u = {borrow()};
            u.p->next = head;
            head = u.p;
        }

        for (int i=0; i<init_count; i++) {
            pvn p;
            p.p = head;
            head = head->next;
            giveback((Object*)p.v);
        }
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
            val.n += +_offset; // back up to the real
            free(val.v);
            ++icount;
        }
        TRACE( TRACE_TRX_FLOW, "Deleted: (%d)\n", icount);
    }


    // Ask for an unused object, if cache empty allocate and return a new one
    Object* borrow() 
    {
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

        // allocates and setups a new object
        vpn u = { malloc(sizeof(Object)) };
        if (!u.v) u.v = null();
        Object* pobj = (Object*)prepare(u);
        pobj->setup(_stl_pools.get());
	return (pobj);
    }
    

    // Returns an object to the cache. The object is reset and put on the
    // free list.
    void giveback(Object* pobj) 
    {        
        assert (pobj);

        // reset the object
        pobj->reset();
        
        // avoid pointer aliasing problems with the optimizer
        union { Object* t; void* v; } u = {pobj};
        
        // give it back
        push(u.v);
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


EXIT_NAMESPACE(dora);


#endif /* __DORA_OBJECT_CACHE_H */
