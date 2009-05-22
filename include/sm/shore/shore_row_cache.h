/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-MT -- Multi-threaded port of the SHORE storage manager
   
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

/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_row_cache.h
 *
 *  @brief:  Cache for tuples (row_impl<>) used in Shore
 *
 *  @note:   row_cache_t           - class for tuple caching
 *
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#ifndef __SHORE_ROW_CACHE_H
#define __SHORE_ROW_CACHE_H

// sparcv9 only at the moment
#include <atomic.h>

#include "util/guard.h"

#include "shore_row_impl.h"


ENTER_NAMESPACE(shore);



// define cache stats to get some statistics about the tuple cache
#undef CACHE_STATS
//#define CACHE_STATS


const int DEFAULT_INIT_ROW_COUNT = 50;


template <class TableDesc>
class row_cache_t : protected atomic_stack
{
public:    
    typedef row_impl<TableDesc> table_tuple;

private:
    TableDesc* _ptable; 
    const int _nbytes;

#ifdef CACHE_STATS    
    // stats
    volatile uint_t _tuple_requests; 
    volatile uint_t _tuple_setups;
#endif              

    // start with a non-empty pool, so that threads don't race
    // at the beginning
    struct mylink {
        table_tuple* _pobj;
        mylink* _next;
        mylink() : _pobj(NULL), _next(NULL) { }
        mylink(table_tuple* obj, mylink* next) : _pobj(obj), _next(next) { }
        ~mylink() { }
    };        

    
    table_tuple* _do_alloc() 
    {
        vpn u = { malloc(_nbytes) };
        if (!u.v) u.v = null();
        table_tuple* pobj = (table_tuple*)prepare(u);
	return (pobj);
    }

public:

    row_cache_t(TableDesc* ptable, int init_count=DEFAULT_INIT_ROW_COUNT) 
        : atomic_stack(-sizeof(ptr)),_nbytes(sizeof(table_tuple)+sizeof(ptr)),
          _ptable(ptable)
#ifdef CACHE_STATS
        , _tuple_requests(0), _tuple_setups(0)
#endif
    { 
        assert (_ptable);
        assert (init_count >= 0); 

        mylink head;
        mylink* node = NULL;

        // create (init_count) objects, and push them to the cache
        for (int i=0; i<init_count; i++) {
            table_tuple* u = borrow();
            node = new mylink(u,head._next);
            head._next = node;
        }

        for (int i=0; i<init_count; i++) {
            giveback(node->_pobj);
            head._next = node;
            node = node->_next;
            delete (head._next);
        }
    }

    
    /* Destroys the cache, calling the destructor for all the objects 
     * it is hoarding.
     */
    ~row_cache_t() 
    {
        vpn val;
        void* v = NULL;
        int icount = 0;
        while ( (v=pop()) ) {
            val.v = v;
            val.n += +_offset; // back up to the real
            ((table_tuple*)v)->~table_tuple();
            free(val.v);
            ++icount;
        }

        TRACE( TRACE_TRX_FLOW, "Deleted: (%d) (%s)\n", icount, _ptable->name());
    }

    /* Return an unused object, if cache empty allocate and return a new one
     */
    table_tuple* borrow() 
    {
#ifdef CACHE_STATS
        atomic_inc_uint(&_tuple_requests);
#endif

        void* val = pop();
        if (val) return ((table_tuple*)(val));

#ifdef CACHE_STATS
        atomic_inc_uint(&_tuple_setups);
#endif

        table_tuple* temp = new (this) table_tuple();
        temp->setup(_ptable);
	return (temp);
    }
    
    /* Returns an object to the cache. The object is reset and put on the
     * free list.
     */
    void giveback(table_tuple* ptn) 
    {        
        assert (ptn);
        assert (ptn->_ptable == _ptable);

        // avoid pointer aliasing problems with the optimizer
        union { table_tuple* t; void* v; } u = {ptn};

        // reset the object
        u.t->reset();
        
        // give it back
        push(u.v);
    }    


#ifdef CACHE_STATS    
    uint_t  setup_count() { return (*&_tuple_setups); }
    uint_t  request_count() { return (*&_tuple_requests); }
    void print_stats() {
        TRACE( TRACE_STATISTICS, "Requests: (%d)\n", *&_tuple_requests);
        TRACE( TRACE_STATISTICS, "Setups  : (%d)\n", *&_tuple_setups);
    }
#endif 

    // these guys need to access the underlying object cache
    template <typename T> 
    friend void* operator new(size_t, row_cache_t<T>*);

    template <typename T> 
    friend void operator delete(void*, row_cache_t<T>*);

}; // EOF: row_cache_t



/* Usage: Object* pobj = new (object_cache_t<Object>) Object(...)
   when finished, call object_cache_t<Object>::destroy(pobj) instead of delete.
 */

template <typename T>
inline void* operator new(size_t nbytes, row_cache_t<T>* cache) 
{
    assert(cache->_nbytes >= nbytes);
    return (cache->_do_alloc());
}


/* Called automatically by the compiler if T's constructor throws
   (otherwise memory would leak).
   Unfortunately, there is no "delete (cache)" syntax in C++ so the user
   must still call cache::destroy()
 */
template <typename T>
inline void operator delete(void* ptr, row_cache_t<T>* cache) 
{
    typedef row_impl<T> table_tuple;
    cache->giveback((table_tuple*)ptr);
}



EXIT_NAMESPACE(shore);


#endif /* __SHORE_ROW_CACHE_H */
