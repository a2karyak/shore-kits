/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __SERVER_CMD_LOAD_HANDLER_H
#define __SERVER_CMD_LOAD_HANDLER_H

#include "core.h"
#include "scheduler.h"
#include "util/command/command_handler.h"

class load_handler_t : public command_handler_t 
{
public:
    virtual void init();
    virtual void handle_command(const char* command);
    virtual void shutdown();    
};

#endif /** __SERVER_CMD_LOAD_HANDLER_H */

