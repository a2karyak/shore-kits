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

}; // EOF: printer_t

#endif /** __UTIL_CMD_PRINTER_H */
