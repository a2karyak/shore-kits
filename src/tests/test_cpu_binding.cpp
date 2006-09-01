/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/thread.h"
#include "trace.h"
#include "tests/common/cpu_bind.h"


using namespace qpipe;


int main(int argc, char* argv[]) {

    thread_init();

    // parse output filename
    if ( argc < 2 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <cpu index>\n", argv[0]);
	exit(-1);
    }
    int cpu_index = atoi(argv[1]);
    if ( cpu_index == 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid cpu_index %s\n", argv[2]);
	exit(-1);
    }

    bind_to_cpu(cpu_index);
    while(1);
    
    return 0;
}
