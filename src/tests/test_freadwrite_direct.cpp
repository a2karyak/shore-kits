/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"
#include "core.h"
#include "scheduler/os_support.h"
#include "workload/tpch/tpch_db.h"
#include "tests/common.h"

#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace qpipe;



int main(int, char**) {

    util_init();

    // parse index
    guard<page> p = page::alloc(sizeof(int));
    for (int i = 0; ; i++) {
        tuple_t tuple((char*)&i, sizeof(i));
        p->append_tuple(tuple);
        if (p->full())
            break;
    }
    
    TRACE(TRACE_ALWAYS, "Filled the page with %zu tuples\n",
          p->tuple_count());
    

    const char* path = "tuple_page_file";


#ifdef FOUND_LINUX
    int file_fd =
        open(path, O_CREAT|O_TRUNC|O_RDWR|O_DIRECT, S_IRUSR|S_IWUSR);
    if (file_fd == -1) {
        TRACE(TRACE_ALWAYS,
              "open(%s, O_CREAT|O_TRUNC|O_RDWR|O_DIRECT, S_IRUSR|S_IWUSR) failed: %s\n",
              path,
              strerror(errno));
        return -1;
    }
    TRACE(TRACE_ALWAYS, "Got fd %d\n", file_fd);
#endif


#ifdef FOUND_SOLARIS
    int file_fd =
        open(path, O_CREAT|O_TRUNC|O_RDWR, S_IRUSR|S_IWUSR);
    if (file_fd == -1) {
        TRACE(TRACE_ALWAYS,
              "open(%s, O_CREAT|O_TRUNC|O_RDWR, S_IRUSR|S_IWUSR) failed: %s\n",
              strerror(errno));
        return -1;
    }
    TRACE(TRACE_ALWAYS, "Got fd %d\n", file_fd);
    int directio_ret = directio(file_fd, DIRECTIO_ON);
    if (directio_ret != 0) {
        TRACE(TRACE_ALWAYS,
              "directio(%d, DIRECTIO_ON) failed: %s\n",
              file_fd,
              strerror(errno));
        return -1;
    }
#endif


    for (int i = 0; i < 10; i++) {

        stopwatch_t wrtime;
        p->write_full_page(file_fd);
        TRACE(TRACE_ALWAYS, "write_full_page took %lf ms\n", wrtime.time_ms());
        
        stopwatch_t seektime;
        int seek_ret = lseek(file_fd, 0, SEEK_SET);
        assert(seek_ret == 0);
        TRACE(TRACE_ALWAYS, "seek took %lf ms\n", seektime.time_ms());
        
        stopwatch_t rdtime;
        bool read_ret = p->read_full_page(file_fd);
        assert(read_ret);
        TRACE(TRACE_ALWAYS, "read_full_page took %lf ms\n", rdtime.time_ms());

        TRACE(TRACE_ALWAYS, "Read a page with %zu tuples\n", p->tuple_count());
    }


    return 0;
}
