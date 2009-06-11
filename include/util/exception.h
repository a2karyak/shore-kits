/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
               Ecole Polytechnique Federale de Lausanne

                       Copyright (c) 2007-2008
                      Carnegie-Mellon University

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
/** @file exception.h
 * 
 *  @brief Miscellaneous exception-related helper functions
 * 
 *  @author Naju Mancheril (ngm)
 */

#ifndef __EXCEPTION_H
#define __EXCEPTION_H

#ifdef __SUNPRO_CC
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#else
#include <cstring>
#include <cerrno>
#include <cassert>
#include <cstdlib>
#endif

#include <exception>

#include "util/c_str.h"

#if 0
#define EXCEPTION(type,__VA_ARGS__) \
   type(__FILE__, __LINE__, __PRETTY_FUNCTION__, c_str(__VA_ARGS__))

#define THROW(type, __VA_ARGS__) \
   throw type(__FILE__, __LINE__, __PRETTY_FUNCTION__, c_str(__VA_ARGS__))

// tests an error code and throws the specified exception if nonzero
#define THROW_IF(Exception, err) \
    do { if(err) throw EXCEPTION(Exception, errno_to_str(err)); } while (0)

#else

#define EXCEPTION(type) \
   type(__FILE__, __LINE__, __PRETTY_FUNCTION__)
#define EXCEPTION1(type, arg) \
   type(__FILE__, __LINE__, __PRETTY_FUNCTION__, c_str(arg))
#define EXCEPTION2(type, arg1, arg2) \
   type(__FILE__, __LINE__, __PRETTY_FUNCTION__, c_str(arg1, arg2))

#define THROW(type) \
   throw type(__FILE__, __LINE__, __PRETTY_FUNCTION__, "")
#define THROW1(type, arg) \
   throw type(__FILE__, __LINE__, __PRETTY_FUNCTION__, c_str(arg))
#define THROW2(type, arg1, arg2) \
   throw type(__FILE__, __LINE__, __PRETTY_FUNCTION__, c_str(arg1, arg2))
// these two only used by cpu_set.cpp
#define THROW3(type, arg1, arg2, arg3) \
   throw type(__FILE__, __LINE__, __PRETTY_FUNCTION__, c_str(arg1, arg2, arg3))
#define THROW4(type, arg1, arg2, arg3, arg4) \
   throw type(__FILE__, __LINE__, __PRETTY_FUNCTION__, c_str(arg1, arg2, arg3, arg4))

// tests an error code and throws the specified exception if nonzero
#define THROW_IF(Exception, err) \
    do { if(err) THROW1(Exception, errno_to_str(err)); } while (0)
#endif


// optional printf-like message allowed here
class SClientException : public std::exception
{

private:

  c_str _message;

public:

  SClientException(const char* filename, int line_num, const char* function_name,
                 c_str const &m)
    : _message(c_str("%s:%d (%s):%s", filename, line_num, function_name, m.data()))
  {
  }

  virtual const char* what() const throw() {
    return _message.data();
  }
 
  virtual ~SClientException() throw() { }
};

#define DEFINE_EXCEPTION(Name) \
    class Name : public SClientException { \
    public: \
        Name(const char* filename, int line_num, const char* function_name, \
             const char* m) \
            : SClientException(filename, line_num, function_name, m) \
            { \
            } \
    }

inline c_str errno_to_str(int err=errno) {
    return strerror(err);
}

DEFINE_EXCEPTION(BadAlloc);
DEFINE_EXCEPTION(OutOfRange);
DEFINE_EXCEPTION(FileException);
DEFINE_EXCEPTION(TrxException);


#ifdef __GCC
inline void unreachable() ATTRIBUTE(noreturn);
inline void unreachable() {
    assert(false);
    exit(-1);
}
#else
#define unreachable() throw "Unreachable"
#endif

#endif
