/* -*- mode:C++; c-basic-offset:4 -*- */
// CC -m64 -xarch=ultraT1 -xs -g -I $SHORE_INCLUDE_DIR shore_tpcc_load.cpp -o shore_tpcc_load -L $SHORE_LIB_DIR -mt -lsm -lsthread -lfc -lcommon -lpthread

#include "tests/common.h"
#include "util/shell.h"

class tree_test_shell_t : shell_t 
{ 
public:

    tree_test_shell_t(const char* prompt = QPIPE_PROMPT) 
        : shell_t(prompt)
        {
        }

    ~tree_test_shell_t() { }


}; // EOF: tree_test_shell_t


int main(int argc, char* argv[]) 
{
    // initialize cordoba threads
    thread_init();

    TRACE_SET( TRACE_ALWAYS | TRACE_STATISTICS | TRACE_NETWORK | TRACE_CPU_BINDING
               //              | TRACE_QUERY_RESULTS
               //              | TRACE_PACKET_FLOW
               //               | TRACE_RECORD_FLOW
               //               | TRACE_TRX_FLOW
               | TRACE_DEBUG
              );


    // test of shell
    shell_t tshell("(ippo)");
    tshell.start();
    return (0);
}



