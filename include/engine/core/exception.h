
#ifndef __EXCEPTION_H
#define __EXCEPTION_H

#include "engine/core/exception/qpipe_exception.h"
#include "engine/core/exception/out_of_memory_exception.h"
#include "engine/core/exception/file_exception.h"
#include "engine/core/exception/terminated_buffer_exception.h"
#include "engine/core/exception/bdb_exception.h"

#define EXCEPTION(type,m) \
   type(__FILE__, __LINE__, __FUNCTION__, m)

#endif
