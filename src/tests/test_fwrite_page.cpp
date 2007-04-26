/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"
#include "core.h"
#include "workload/tpch/tpch_db.h"
#include "tests/common.h"

#include <cstdlib>

using namespace qpipe;



int main(int, char**) {

    util_init();

    // parse index
    guard<qpipe::page> p = qpipe::page::alloc(sizeof(int));
    for (int i = 0; ; i++) {
        tuple_t tuple((char*)&i, sizeof(i));
        p->append_tuple(tuple);
        if (p->full())
            break;
    }
    
    TRACE(TRACE_ALWAYS, "Filled the page with %zu tuples\n",
          p->tuple_count());
    
    guard<FILE> file = fopen("tuple_page_file", "w+");
    assert(file != NULL);
    p->fwrite_full_page(file);
    

    return 0;
}
