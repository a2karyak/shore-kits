/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util/trace.h"
#include "util/util_init.h"
#include "util/busy_delay.h"


/* definitions of exported functions */

void util_init(void)
{
    thread_init();
    TRACE(TRACE_ALWAYS, "Running util_init()\n");
    busy_delay_init();
}
