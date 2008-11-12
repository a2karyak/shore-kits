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

    void close() { assert(0); }
    const int handle(const char* cmd) { return (SHELL_NEXT_CONTINUE); }
    void setaliases() { assert(0); }
    void usage() { assert(0); }
    const string desc() { return (string("")); }               

};

#endif /** __SERVER_CMD_LOAD_HANDLER_H */

