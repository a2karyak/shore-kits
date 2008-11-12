/* -*- mode:C++; c-basic-offset:4 -*- */


#include "util.h"
#include "util/command/printer.h"


#ifdef __SUNPRO_CC
#include <string.h>
#include <stdio.h>
#else
#include <cstring>
#include <cstdio>
#endif


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
