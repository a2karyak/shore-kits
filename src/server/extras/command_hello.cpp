/* -*- mode:C++; c-basic-offset:4 -*- */


#include "core.h"
#include "server/command/command_handler.h"
#include <cstring>
#include <cstdio>

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
