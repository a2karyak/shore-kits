/* -*- mode:C++; c-basic-offset:4 -*- */

#include "tests/common.h"
#include "util/shell.h"


class tree_shell : public shell_t
{
public:

    tree_shell()
        : shell_t("(allou) ")
    { }

    ~tree_shell() 
    { 
        TRACE( TRACE_ALWAYS, "Exiting... (%d) commands processed\n", 
               get_command_cnt());
    }

    int process_command(const char* cmd);
    int print_usage(const char* cmd);

}; // EOF: class tree_shell


int tree_shell::process_command(const char* cmd) 
{
    TRACE( TRACE_ALWAYS, "(%s)\n", cmd);
    return (SHELL_NEXT_CONTINUE); 
}


int tree_shell::print_usage(const char* cmd) 
{ 
    TRACE( TRACE_ALWAYS, "helping in (%s)\n", cmd);
    return (SHELL_NEXT_CONTINUE); 
}


/////////////////////////////////////////////////////////////



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


    tree_shell ak;
    ak.start();

    return (0);
}



