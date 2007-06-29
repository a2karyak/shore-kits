/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file mem_obj.h
 *
 *  @brief Memory object definition
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __QPIPE_MEM_OBJ_H
#define __QPIPE_MEM_OBJ_H

#include "util/namespace.h"


ENTER_NAMESPACE(qpipe);

class memObject_t {

protected:

  int _allocated;

public:

    /** Construction */

    memObject_t() { 
      _allocated = 0;
    }
    
    virtual ~memObject_t() { }

    /** Access functions */

    inline int is_allocated() { return (_allocated); }

    
    /** Interface */
    
    virtual void allocate() { }
    
    virtual void deallocate() { }
  
    virtual void reset() { }

}; // EOF memObject_t


EXIT_NAMESPACE(qpipe);

#endif
