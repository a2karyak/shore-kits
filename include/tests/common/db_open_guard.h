/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __DB_OPEN_GUARD_H
#define __DB_OPEN_GUARD_H

#include "workload/tpch/tpch_db.h"


struct db_open_guard_t {

    db_open_guard_t(uint32_t flags=0,
                    uint32_t db_cache_size_gb=1,
                    uint32_t db_cache_size_bytes=0)
    {
        db_open(flags, db_cache_size_gb, db_cache_size_bytes);
    }

    ~db_open_guard_t() {
        db_close();
    }

};


#endif
