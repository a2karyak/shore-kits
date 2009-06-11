/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
               Ecole Polytechnique Federale de Lausanne

                       Copyright (c) 2007-2008
                      Carnegie-Mellon University
   
   Permission to use, copy, modify and distribute this software and
   its documentation is hereby granted, provided that both the
   copyright notice and this permission notice appear in all copies of
   the software, derivative works or modified versions, and any
   portions thereof, and that both notices appear in supporting
   documentation.
   
   This code is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
   DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
   RESULTING FROM THE USE OF THIS SOFTWARE.
*/


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
