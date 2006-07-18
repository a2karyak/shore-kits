/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _PRINTER_H
#define _PRINTER_H

#include "trace.h"
#include <cstdio>



class printer_t : public command_handler_t {

public:

    virtual void init() {
        // do nothing
    }

    virtual void handle(const char* command) {

        int num;
        if ( sscanf(command, "%*s %d", &num) < 1 )
            TRACE(TRACE_ALWAYS, "num not specified... aborting\n");

        for (int i = 0; i < num; i++) {
            PRINT("%s", command);
        }
    }
    
};



#endif
