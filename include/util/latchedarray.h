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

    pthread_rwlock_t _array_rwlock[SF * FANOUT];

public:

    // Constructor
    latchedArray() {
        // initializes the locks
        for (int i=0; i<SF*FANOUT; i++) {
            pthread_rwlock_init(&_array_rwlock[i], NULL);
        }
    }

    // Destructor
    virtual ~latchedArray() {
        // destroys the locks
        for (int i=0; i<SF*FANOUT; i++) {
            pthread_rwlock_destroy(&_array_rwlock[i]);
        }
    }        

    /** Exported Functions */

    /** @fn read
     *
     *  @brief Returns the requested entry locked for read
     *
     *  @return A pointer to the read-locked entry of the array, NULL on error
     */

    TUPLE_TYPE* read(int idx) {
      if ( (idx >= 0) && (idx < SF*FANOUT) ) {
        pthread_rwlock_rdlock(&_array_rwlock[idx]);
        return (&_array[idx]);
      }

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
        pthread_rwlock_wrlock(&_array_rwlock[idx]);
        return (&_array[idx]);
      }

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
        pthread_rwlock_unlock(&_array_rwlock[idx]);
        return (0);
      }

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
        pthread_rwlock_wrlock(&_array_rwlock[idx]);
        _array[idx] = anEntry;
        pthread_rwlock_unlock(&_array_rwlock[idx]);        
        return (0);
      }

      // out-of-bounds
      return (1);
    }

}; // EOF latchedArray


#endif 
