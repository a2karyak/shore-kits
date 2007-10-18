/* -*- mode:C++; c-basic-offset:4 -*- */

#include "core.h"
#include "server/command/command_handler.h"

#ifdef __SUNPRO_CC
#include <string.h>
#include <stdio.h>
#else
#include <cstring>
#include <cstdio>
#endif

class hello_handler_t : public command_handler_t {
public:
    virtual void init() { }
    virtual void shutdown() { }
    virtual void handle_command(const char*) {
        printf("Hello world!\n");
    }
};

// the hook for dlsym
extern "C"
command_handler_t* hello() {
    return new hello_handler_t();
}
