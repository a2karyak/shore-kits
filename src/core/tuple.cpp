/* -*- mode:C++; c-basic-offset:4 -*- */
#include <cstdio>

#include "core/tuple.h"


ENTER_NAMESPACE(qpipe);



malloc_page_pool malloc_page_pool::_instance;



static size_t default_page_size = 8192;

static bool initialized = false;

void set_default_page_size(size_t page_size) {
    assert(!initialized);
    initialized = true;
    default_page_size = page_size;
}

size_t get_default_page_size() { return default_page_size; }



bool page::fread_full_page(FILE* file) {

    // save page attributes that we'll be overwriting
    size_t size = page_size();
    TRACE(0&TRACE_ALWAYS, "Computed page size as %d\n", (int)size);
    page_pool* pool = _pool;

    // write over this page
    size_t size_read = ::fread(this, 1, size, file);
    _pool = pool;
        
    // We expect to read either a whole page or no data at
    // all. Anything else is an error.
    if ( (size_read == 0) && feof(file) )
        // done with file
        return false;

    // check sizes match
    if ( (size != size_read) || (size != page_size()) ) {
        // The page we read does not have the same size as the
        // page object we overwrote. Luckily, we used the object
        // size when reading, so we didn't overflow our internal
        // buffer.
        TRACE(TRACE_ALWAYS,
              "Read %zd byte-page with internal page size of %zd bytes into a buffer of %zd bytes. "
              "Sizes should all match.\n",
              size_read,
              page_size(),
              size);
        THROW1(FileException, "::fread read wrong size page");
    }

    return true;
}



void page::fwrite_full_page(FILE *file) {
    size_t write_count = ::fwrite(this, 1, page_size(), file);
    if ( write_count != page_size() ) {
        TRACE(TRACE_ALWAYS, "::fwrite() wrote %zd/%zd page bytes\n",
              write_count,
              page_size());
        THROW1(FileException, "::fwrite() failed");
    }
}



EXIT_NAMESPACE(qpipe);
