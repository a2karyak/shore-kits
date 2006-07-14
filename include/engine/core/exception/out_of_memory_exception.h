
#ifndef __OUT_OF_MEMORY_EXCEPTION_H
#define __OUT_OF_MEMORY_EXCEPTION_H

#include "engine/core/exception/qpipe_exception.h"


#include "engine/namespace.h"


class OutOfMemoryException : public QPipeException {

 public:

  OutOfMemoryException(const string& m,
                       const char* filename, int line_num, const char* function_name)
    : QPipeException(m, filename, line_num, function_name)
    {
    }
};


#include "engine/namespace.h"


#endif
