/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _PRINTER_H
#define _PRINTER_H

#include "server/command/command_handler.h"



class printer_t : public command_handler_t {

public:

    virtual void handle_command(const char* command);

    virtual ~printer_t() { }
};



#endif
