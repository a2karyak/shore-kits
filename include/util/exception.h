// -*- mode:C++; c-basic-offset:4 -*-
#ifndef __EXCEPTION_H
#define __EXCEPTION_H

#include <exception>
#include <cstring>
#include <cerrno>

#include "util/c_str.h"

// optional printf-like message allowed here
#define EXCEPTION(type,args...) \
   type(__FILE__, __LINE__, __PRETTY_FUNCTION__, c_str(args))

// tests an error code and throws the specified exception if nonzero
#define THROW_IF(Exception, err) \
    do { if(err) throw EXCEPTION(Exception, errno_to_str(err)); } while (0)

class QPipeException : public std::exception
{

private:

  c_str _message;

public:

  QPipeException(const char* filename, int line_num, const char* function_name,
                 c_str const &m)
    : _message(c_str("%s:%d (%s):%s", filename, line_num, function_name, m.data()))
  {
  }

  virtual const char* what() const throw() {
    return _message.data();
  }
 
  virtual ~QPipeException() throw() { }
};

#define DEFINE_EXCEPTION(Name) \
    class Name : public QPipeException { \
    public: \
        Name(const char* filename, int line_num, const char* function_name, \
             const char* m) \
            : QPipeException(filename, line_num, function_name, m) \
            { \
            } \
    }

inline c_str errno_to_str(int err=errno) {
    return strerror(err);
}

DEFINE_EXCEPTION(BadAlloc);
DEFINE_EXCEPTION(OutOfRange);
DEFINE_EXCEPTION(FileException);
DEFINE_EXCEPTION(BdbException);

#endif
