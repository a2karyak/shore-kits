/* -*- mode:C++; c-basic-offset:4 -*- */

#include "core/tuple.h"


ENTER_NAMESPACE(qpipe);

malloc_page_pool malloc_page_pool::_instance;


static size_t default_page_size = 4096;
static bool initialized = false;

void set_default_page_size(size_t page_size) {
    assert(!initialized);
    initialized = true;
    default_page_size = page_size;
}

size_t get_default_page_size() { return default_page_size; }


EXIT_NAMESPACE(qpipe);
