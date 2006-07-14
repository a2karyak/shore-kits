
#ifndef __OUT_OF_MEMORY_EXCEPTION_H
#define __OUT_OF_MEMORY_EXCEPTION_H

#include "engine/core/exception/qpipe_exception.h"


#include "engine/namespace.h"


class OutOfMemoryException : public QPipeException {

 public:

  OutOfMemoryException(const char* filename, int line_num, const char* function_name,
                       const char* m)
    : QPipeException(filename, line_num, function_name, m)
    {
    }
};


#include "engine/namespace.h"


#endif
