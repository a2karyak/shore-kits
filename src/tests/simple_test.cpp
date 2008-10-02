/* -*- mode:C++; c-basic-offset:4 -*- */

#include "tests/common.h"

#include "tests/common/tester_shore_kit_shell.h"

#include "stages/tpcc/shore/shore_tpcc_env.h"
#include "sm/shore/shore_helper_loader.h"

#include "dora.h"

using namespace dora;


//class simple_shore_shell : public shell_t
class simple_shore_shell : public shore_kit_shell_t
{
public:

    simple_shore_shell()
        : shore_kit_shell_t("(allou) ", NULL)
    { }

    ~simple_shore_shell() 
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
    
}; // EOF: class simple_shore_shell



int simple_shore_shell::_cmd_WARMUP_impl(const int iQueriedWHs, 
                                         const int iTrxs, 
                                         const int iDuration, 
                                         const int iIterations)
{
    TRACE( TRACE_ALWAYS, "warming...\n");
    return (SHELL_NEXT_CONTINUE);
}


int simple_shore_shell::_cmd_TEST_impl(const int iQueriedWHs, 
                                       const int iSpread,
                                       const int iNumOfThreads, 
                                       const int iNumOfTrxs,
                                       const int iSelectedTrx, 
                                       const int iIterations,
                                       const int iUseSLI)
{
    typedef key_wrapper_t<int> intkey;
    intkey a,b;
    std::less<intkey> sls;


    TRACE( TRACE_ALWAYS, "testing... (%d) commands processed\n",
           get_command_cnt());

    range_partition_impl<int> aCustPart(_g_shore_env, _g_shore_env->customer(), 3);
    tid_t atid;
    int y=0;
    for (int i=0; i<iNumOfTrxs; i++) {
        TRACE( TRACE_DEBUG, "false\n");
        key_wrapper_t<int> akey;
        akey.push_back(i);

        key_wrapper_t<int> bkey;
        bkey.push_back(y);

        if (akey<bkey) {
            TRACE( TRACE_DEBUG, "true\n");
        }
        else {
            TRACE( TRACE_DEBUG, "false\n");
        }
        
        aCustPart.plm()->acquire(atid, akey);        
    }
    aCustPart.plm()->release(atid);
    aCustPart.reset();

    return (SHELL_NEXT_CONTINUE);
}



/** cmd: MEASURE **/

int simple_shore_shell::_cmd_MEASURE_impl(const int iQueriedWHs, 
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


    /* 1. Instanciate the Shore environment */
    if (inst_test_env(argc, argv))
        return (1);

    // start processing commands
    simple_shore_shell sss;
    sss.start();

    /* 3. Close the Shore environment */
    if (_g_shore_env)
        close_test_env();

    return (0);
}



