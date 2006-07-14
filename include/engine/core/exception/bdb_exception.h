
#ifndef __BDB_EXCEPTION_H
#define __BDB_EXCEPTION_H

#include "engine/core/exception/qpipe_exception.h"


#include "engine/namespace.h"


class Berkeley_DB_Exception : public QPipeException {

 public:

  Berkeley_DB_Exception(const char* filename, int line_num, const char* function_name,
                        const char* m)
    : QPipeException(filename, line_num, function_name, m)
    {
    }
};


#include "engine/namespace.h"


#endif
