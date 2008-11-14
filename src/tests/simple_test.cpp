/* -*- mode:C++; c-basic-offset:4 -*- */

#include "tests/common.h"

#include "tests/common/tester_shore_shell.h"

#include "workload/tpcc/shore_tpcc_env.h"
#include "sm/shore/shore_helper_loader.h"

#include "dora.h"
#include "dora/tpcc/dora_tpcc.h"


using namespace dora;
using namespace tpcc;




int main(int argc, char* argv[]) 
{
    thread_init();


    TRACE_SET( TRACE_ALWAYS | TRACE_STATISTICS | TRACE_NETWORK | TRACE_CPU_BINDING
               //              | TRACE_QUERY_RESULTS
               //              | TRACE_PACKET_FLOW
               //               | TRACE_RECORD_FLOW
               //               | TRACE_TRX_FLOW
               | TRACE_DEBUG
              );


    return (0);
}



