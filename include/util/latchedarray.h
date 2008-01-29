/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file latchedarray.h
 *
 *  @brief Implementation of a simple array with latches
 *
 *  @uses Uses pthread_mutexes
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __LATCHED_ARRAY_H
#define __LATCHED_ARRAY_H


#include <boost/array.hpp>


#include "util/sync.h"
#include "util/trace.h"
#include "util/clh_lock.h"


//#define LARRAY_USE_MUTEX
#define LARRAY_USE_CLH_LOCK

// undef macros
#undef INIT
#undef DESTROY
#undef ACQUIRE_READ
#undef ACQUIRE_WRITE
#undef RELEASE


#if defined(LARRAY_USE_MUTEX)
typedef pthread_mutex_t LArrayLock;
#define INIT(lock) pthread_mutex_init(&lock, NULL)
#define DESTROY(lock) pthread_mutex_destroy(&lock)
#define ACQUIRE_READ(lock) pthread_mutex_lock(&lock)
#define ACQUIRE_WRITE(lock) pthread_mutex_lock(&lock)
#define RELEASE(lock) pthread_mutex_unlock(&lock)

#elif defined(LARRAY_USE_CLH_LOCK)
typedef clh_lock LArrayLock;
#define INIT(lock)
#define DESTROY(lock)
#define ACQUIRE_READ(lock) lock.acquire()
#define ACQUIRE_WRITE(lock) lock.acquire()
#define RELEASE(lock) lock.release()

#else
typedef  pthread_rwlock_t LArrayLock;
#define INIT(lock) pthread_rwlock_init(&lock, NULL)
#define DESTROY(lock) pthread_rwlock_destroy(&lock)
#define ACQUIRE_READ(lock) pthread_rwlock_rdlock(&lock)
#define ACQUIRE_WRITE(lock) pthread_rwlock_wrlock(&lock)
#define RELEASE(lock) pthread_rwlock_unlock(&lock)
#endif



////////////////////////////////////////////////////////////////////////////

/** @class latchedArray
 *
 *  @brief Implementation of a class that regulates the accesses to an array 
 *  of elements with a pthread_rwlock for each entry.
 *
 *  @note The functions return the objects locked accordingly. 
 *  It is responsibility of the caller to release them.
 */

template <typename TUPLE_TYPE, int SF, int FANOUT>
class latchedArray
{
private:

    boost::array<TUPLE_TYPE, SF * FANOUT> _array;

    LArrayLock _array_rwlock[SF * FANOUT];
    c_str _name;

public:

    // Constructor
    latchedArray(c_str name="<nameless>")
	: _name(name)
    {
        // initializes the locks
        for (int i=0; i<SF*FANOUT; i++) {
            INIT(_array_rwlock[i]);
        }
    }

    // Destructor
    virtual ~latchedArray() {
        // destroys the locks
        for (int i=0; i<SF*FANOUT; i++) {
            DESTROY(_array_rwlock[i]);
        }
    }        

    /** Exported Functions */

    c_str get_name() { return _name; }
    void set_name(c_str name) { _name = name; }

    /** @fn read
     *
     *  @brief Returns the requested entry locked for read
     *
     *  @return A pointer to the read-locked entry of the array, NULL on error
     */

    TUPLE_TYPE* read(int idx) {
      if ( (idx >= 0) && (idx < SF*FANOUT) ) {
	  ACQUIRE_READ(_array_rwlock[idx]);
        return (&_array[idx]);
      }

      TRACE( TRACE_TRX_FLOW, "Out-of-bounds access attempt idx = (%d)\n", idx);

      // out-of-bounds
      return (NULL);
    }


    /** @fn write
     *
     *  @brief Returns the requested entry locked for write
     *
     *  @return A pointer to the write-locked entry of the array, NULL on error
     */

    TUPLE_TYPE* write(int idx) {
      if ( (idx >= 0) && (idx < SF*FANOUT) ) {
	  ACQUIRE_WRITE(_array_rwlock[idx]);
        return (&_array[idx]);
      }

      TRACE( TRACE_TRX_FLOW, "Out-of-bounds access attempt idx = (%d)\n", idx);

      // out-of-bounds
      return (NULL);
    }


    /** @fn release
     *
     *  @brief Releases a lock for the requested entry
     *
     *  @return 0 on success, non-zero otherwise
     */

    int release(int idx) {
      if ( (idx >= 0) && (idx < SF*FANOUT) ) {
	  RELEASE(_array_rwlock[idx]);
        return (0);
      }

      TRACE( TRACE_TRX_FLOW, "Out-of-bounds access attempt idx = (%d)\n", idx);

      // out-of-bounds
      return (1);
    }


    /** @fn insert
     *
     *  @brief  Inserts a new entry in the specified place of the array
     *
     *  @return 0 on success, non-zero otherwise
     */

    int insert(int idx, const TUPLE_TYPE anEntry) {
      if ( (idx >= 0) && (idx < SF*FANOUT) ) {
	  ACQUIRE_WRITE(_array_rwlock[idx]);
        _array[idx] = anEntry;
        RELEASE(_array_rwlock[idx]);        
        return (0);
      }

      TRACE( TRACE_TRX_FLOW, "Out-of-bounds access attempt idx = (%d)\n", idx);

      // out-of-bounds
      return (1);
    }



    /** @fn read_nl
     *
     *  @brief Reads the requested entry without locking
     *
     *  @return A pointer to the  entry of the array, NULL on error
     */

    TUPLE_TYPE* read_nl(int idx) {
      if ( (idx >= 0) && (idx < SF*FANOUT) ) {
        return (&_array[idx]);
      }

      TRACE( TRACE_TRX_FLOW, "Out-of-bounds access attempt idx = (%d)\n", idx);

      // out-of-bounds
      return (NULL);
    }


    /** @fn insert_nl
     *
     *  @brief  Inserts a new entry in the specified place of the array
     *  without locking
     *
     *  @note Should be very careful when doing such an operation
     *
     *  @return 0 on success, non-zero otherwise
     */

    int insert_nl(int idx, const TUPLE_TYPE anEntry) {
      if ( (idx >= 0) && (idx < SF*FANOUT) ) {
        _array[idx] = anEntry;
        return (0);
      }

      TRACE( TRACE_TRX_FLOW, "Out-of-bounds access attempt idx = (%d)\n", idx);

      // out-of-bounds
      return (1);
    }   




    /** @fn dump
     *
     *  @brief Dumps the contents of the array
     *
     *  @note Very inefficient, should used only for debugging
     */

    void dump() {
        for (int idx=0; idx<SF*FANOUT; idx++) {
	    ACQUIRE_READ(_array_rwlock[idx]);
            TRACE( TRACE_DEBUG, "%d %s\n", idx, _array[idx].tuple_to_str().data());
            RELEASE(_array_rwlock[idx]);        
        }
    }

    void save(FILE* fout) {
	for(int i=0; i < SF*FANOUT; i++) {
	    int count = fwrite(read(i), sizeof(TUPLE_TYPE), 1, fout);
	    release(i);
	    if(count != 1)
		THROW_IF(FileException, errno);
	}
    }
    void restore(FILE* fin) {
	for(int i=0; i < SF*FANOUT; i++) {
	    int count = fread(write(i), sizeof(TUPLE_TYPE), 1, fin);
	    release(i);
	    if(count != 1)
		THROW_IF(FileException, errno);
	}
    }
}; // EOF latchedArray

#undef INIT
#undef DESTROY
#undef ACQUIRE_READ
#undef ACQUIRE_WRITE
#undef RELEASE

#endif 
