/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _CONFIG_COMMAND_H
#define _CONFIG_COMMAND_H

#include <map>
#include "server/command/command_handler.h"
#include "util.h"

using std::map;


class config_command_t : public command_handler_t {

    map<c_str, bool*> _known_config_vars;
    
public:

    virtual void init();
    virtual void handle_command(const char* command);
    virtual ~config_command_t() { }

private:
    
    void enable(const char* var);
    void disable(const char* var);
    void print_enabled_vars();
    void print_known_vars();
    void print_usage(const char* command_tag);
};



#endif
