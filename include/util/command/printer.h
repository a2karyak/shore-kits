/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __UTIL_CMD_PRINTER_H
#define __UTIL_CMD_PRINTER_H


#include "util/command/command_handler.h"


class printer_t : public command_handler_t 
{
public:
    printer_t() { }
    virtual ~printer_t() { }

    virtual void handle_command(const char* command);


    void init() { assert(0); }
    void close() { assert(0); }
    const int handle(const char* cmd) { return (SHELL_NEXT_CONTINUE); }
    void setaliases() { assert(0); }
    void usage() { assert(0); }
    const string desc() { return (string("")); }               

}; // EOF: printer_t

#endif /** __UTIL_CMD_PRINTER_H */
