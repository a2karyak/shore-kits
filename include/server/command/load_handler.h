/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef __LOAD_HANDLER_H
#define __LOAD_HANDLER_H

#include "core.h"
#include "scheduler.h"
#include "server/command/command_handler.h"

class load_handler_t : public command_handler_t {
public:
    virtual void init();
    virtual void handle_command(const char* command);
    virtual void shutdown();
    
};
#endif
