
#ifndef _TPCH_DB_H
#define _TPCH_DB_H

#include "tests/common/tpch_env.h"
#include "tests/common/tpch_db_load.h"
#include <db_cxx.h>


bool db_open(u_int32_t flags=DB_RDONLY|DB_THREAD);
bool db_close();


#endif
