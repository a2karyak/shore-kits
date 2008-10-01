/* -*- mode:C++; c-basic-offset:4 -*- */

#include "tests/common.h"
//#include "util/shell.h"
#include "tests/common/tester_shore_kit_shell.h"


//class tree_shell : public shell_t
class tree_shell : public shore_kit_shell_t
{
public:

    tree_shell()
        : shore_kit_shell_t("(allou) ", NULL)
    { }

    ~tree_shell() 
    { 
        TRACE( TRACE_ALWAYS, "Exiting... (%d) commands processed\n",
               get_command_cnt());
    }


//     int process_command(const char* cmd);
//     int print_usage(const char* cmd);


    // impl of supported commands
    virtual int _cmd_WARMUP_impl(const int iQueriedWHs, const int iTrxs, 
                                 const int iDuration, const int iIterations);
    virtual int _cmd_TEST_impl(const int iQueriedWHs, const int iSpread,
                               const int iNumOfThreads, const int iNumOfTrxs,
                               const int iSelectedTrx, const int iIterations,
                               const int iUseSLI);
    virtual int _cmd_MEASURE_impl(const int iQueriedWHs, const int iSpread,
                                  const int iNumOfThreads, const int iDuration,
                                  const int iSelectedTrx, const int iIterations,
                                  const int iUseSLI);    
    
}; // EOF: class tree_shell


// int tree_shell::process_command(const char* cmd) 
// {
//     TRACE( TRACE_ALWAYS, "(%s)\n", cmd);
//     return (SHELL_NEXT_CONTINUE); 
// }


// int tree_shell::print_usage(const char* cmd) 
// { 
//     TRACE( TRACE_ALWAYS, "helping in (%s)\n", cmd);
//     return (SHELL_NEXT_CONTINUE); 
// }

int tree_shell::_cmd_WARMUP_impl(const int iQueriedWHs, const int iTrxs, 
                                 const int iDuration, const int iIterations)
{
    TRACE( TRACE_ALWAYS, "warming...\n");
    return (SHELL_NEXT_CONTINUE);
}


int tree_shell::_cmd_TEST_impl(const int iQueriedWHs, 
                               const int iSpread,
                               const int iNumOfThreads, 
                               const int iNumOfTrxs,
                               const int iSelectedTrx, 
                               const int iIterations,
                               const int iUseSLI)
{
    TRACE( TRACE_ALWAYS, "testing... (%d) commands processed\n",
           get_command_cnt());

    return (SHELL_NEXT_CONTINUE);
}



/** cmd: MEASURE **/

int tree_shell::_cmd_MEASURE_impl(const int iQueriedWHs, 
                                  const int iSpread,
                                  const int iNumOfThreads, 
                                  const int iDuration,
                                  const int iSelectedTrx, 
                                  const int iIterations,
                                  const int iUseSLI)
{
    TRACE( TRACE_ALWAYS, "measuring... (%d) commands processed\n",
           get_command_cnt());
    
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



