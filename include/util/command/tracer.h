/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __UTIL_CMD_TRACER_H
#define __UTIL_CMD_TRACER_H


#include <map>
#include "util/command/command_handler.h"
#include "util.h"

using std::map;


class tracer_t : public command_handler_t 
{
    map<c_str, int> _known_types;
    
public:

    tracer_t() { init(); }
    virtual ~tracer_t() { }

    virtual void init();
    virtual void handle_command(const char* command);


private:
    
    void enable(const char* type);
    void disable(const char* type);
    void print_enabled_types();
    void print_known_types();
    void print_usage(const char* command_tag);

}; // EOF: tracer_t


#endif /** __UTIL_CMD_TRACER_H */


