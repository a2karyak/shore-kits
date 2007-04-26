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


    int fseek_ret = fseek(file, 0, SEEK_SET);
    assert(fseek_ret == 0);
    bool read_ret = p->fread_full_page(file);
    assert(read_ret);

    TRACE(TRACE_ALWAYS, "Read a page with %zu tuples\n", p->tuple_count());
    for (size_t i = 0; i < p->tuple_count(); i++) {
        tuple_t output;
        output = p->get_tuple(i);
        int* t = aligned_cast<int>(output.data);
        TRACE(TRACE_ALWAYS, "Read tuple value %d\n", *t);
    }

    return 0;
}
