/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _TRACER_H
#define _TRACER_H

#include <map>
#include "server/command/command_handler.h"
#include "util.h"

using std::map;


class tracer_t : public command_handler_t {

    map<c_str, int> _known_types;
    
public:

    virtual void init();
    virtual void handle_command(const char* command);
    virtual ~tracer_t() { }

private:
    
    void enable(const char* type);
    void disable(const char* type);
    void print_enabled_types();
    void print_known_types();
    void print_usage(const char* command_tag);
};



#endif
