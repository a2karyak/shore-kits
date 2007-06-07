/* -*- mode:C++; c-basic-offset:4 -*- */


#include "util.h"
#include "server/command/printer.h"
#include "server/print.h"
#include <cstring>
#include <cstdio>



void printer_t::handle_command(const char* command) {

    int num;
    if ( sscanf(command, "%*s %d", &num) < 1 ) {
        TRACE(TRACE_ALWAYS, "num not specified... aborting\n");
        return;
    }
    
    for (int i = 0; i < num; i++) {
        TRACE(TRACE_ALWAYS, "%s\n", command);
    }
}
