
#ifndef __QPIPE_EXCEPTION_H
#define __QPIPE_EXCEPTION_H

#include "engine/util/c_str.h"
#include <exception>
#include <string>
#include <sstream>


using std::exception;
using std::string;
using std::ostringstream;



#include "engine/namespace.h"



class QPipeException : public std::exception
{

private:

  c_str _message;

public:

  QPipeException(const char* filename, int line_num, const char* function_name,
                 const char* m)
    : _message(c_str("%s:%s:%d:%s", filename, function_name, line_num, m))
  {
  }

  virtual const char* what() const throw() {
    return _message.data();
  }
 
  virtual ~QPipeException() throw() { }
};



#include "engine/namespace.h"



#endif
