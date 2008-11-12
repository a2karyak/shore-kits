/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __UTIL_CMD_TRACE_H
#define __UTIL_CMD_TRACE_H


#include <map>
#include "util/command/command_handler.h"
#include "util.h"

using std::map;


class trace_cmd_t : public command_handler_t 
{
    map<c_str, int> _known_types;
    
public:

    trace_cmd_t() { init(); }
    ~trace_cmd_t() { }

    void init();
    void close() { }

    const int handle(const char* cmd);

    void setaliases();
    void usage();
    const string desc() { return (string("Manipulates tracing level")); }               

private:
    
    void enable(const char* type);
    void disable(const char* type);
    void print_enabled_types();
    void print_known_types();

}; // EOF: trace_cmd_t


#endif /** __UTIL_CMD_TRACE_H */


