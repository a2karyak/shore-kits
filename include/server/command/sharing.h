/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _SHARING_H
#define _SHARING_H

#include <map>
#include "server/command/command_handler.h"
#include "util.h"


class sharing_t : public command_handler_t {

public:

    virtual void init();
    virtual void handle_command(const char* command);
    virtual ~sharing_t() { }

private:
    
    void global_enable();
    void global_disable();
    c_str global_sharing_policy();
    void print_usage(const char* command_tag);
};



#endif
