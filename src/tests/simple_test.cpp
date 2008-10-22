/* -*- mode:C++; c-basic-offset:4 -*- */

#include "tests/common.h"

#include "tests/common/tester_shore_kit_shell.h"

#include "stages/tpcc/shore/shore_tpcc_env.h"
#include "sm/shore/shore_helper_loader.h"

#include "dora.h"
#include "dora/tpcc/dora_tpcc.h"


using namespace dora;
using namespace tpcc;



//class simple_shore_shell : public shell_t
class simple_shore_shell : public shore_kit_shell_t
{
public:

    simple_shore_shell(ShoreTPCCEnv* tpccenv)
        : shore_kit_shell_t("(allou) ", tpccenv)
    { }

    ~simple_shore_shell() 
    { 
        TRACE( TRACE_ALWAYS, "Exiting... (%d) commands processed\n",
               get_command_cnt());
    }


    // impl of supported commands
    virtual int _cmd_WARMUP_impl(const int iQueriedWHs, const int iTrxs, 
                                 const int iDuration, const int iIterations);
    virtual int _cmd_TEST_impl(const int iQueriedWHs, const int iSpread,
                               const int iNumOfThreads, const int iNumOfTrxs,
                               const int iSelectedTrx, const int iIterations,
                               const int iUseSLI, const eBindingType abt);
    virtual int _cmd_MEASURE_impl(const int iQueriedWHs, const int iSpread,
                                  const int iNumOfThreads, const int iDuration,
                                  const int iSelectedTrx, const int iIterations,
                                  const int iUseSLI, const eBindingType abt);    
    
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
                                       const int iUseSLI,
                                       const eBindingType abt)
{
    TRACE( TRACE_ALWAYS, "testing... (%d) commands processed\n",
           get_command_cnt());

    // does (iIterations) iterations
    for (int i=0; i<iIterations; i++) {

        // get a "trx_id"
        _g_dora->cus(0)->dump();    
        tid_t atid(i,0);
        cout << atid << endl;

        // for every transaction acquires (iSelectedTrx) random keys
        for (int j=0;j<iSelectedTrx;j++) {            

            DoraLockMode adlm = DoraLockModeArray[rand()%DL_CC_MODES]; // random lmode
            dora_tpcc_db::ikey akey;
            int wh = URand(1,iQueriedWHs);
            int d = URand(1, 10);
            int c = 1;//NURand(1023, 1, 3000);
            akey.push_back(wh); // WH
            akey.push_back(d); // DISTR
            akey.push_back(c); // CUST

            TRACE( TRACE_DEBUG, "TRX (%d) - K (%d|%d|%d) - LM (%d)\n", 
                   atid, wh, d, c, adlm);
            
            // acquire
            if (_g_dora->cus(0)->plm()->acquire(atid,akey,adlm)) {
                TRACE( TRACE_DEBUG, "TRX (%d) - K (%d|%d|%d) - LM (%d) - ACQUIRED\n", 
                       atid, wh, d, c, adlm);
            }
            else {
                TRACE( TRACE_DEBUG, "TRX (%d) - K (%d|%d|%d) - LM (%d) - FAILED\n", 
                       atid, wh, d, c, adlm);
            }
        }
        // release all
        _g_dora->cus(0)->plm()->dump();
        _g_dora->cus(0)->plm()->release(atid);
    }

    // should be empty here!
    _g_dora->cus(0)->plm()->dump();
    return (SHELL_NEXT_CONTINUE);
}



/** cmd: MEASURE **/

int simple_shore_shell::_cmd_MEASURE_impl(const int iQueriedWHs, 
                                          const int iSpread,
                                          const int iNumOfThreads, 
                                          const int iDuration,
                                          const int iSelectedTrx, 
                                          const int iIterations,
                                          const int iUseSLI,
                                          const eBindingType abt)
{
    TRACE( TRACE_ALWAYS, "measuring... (%d) commands processed\n",
           get_command_cnt());

    _g_dora->cus(0)->reset();
    _g_dora->cus(0)->dump();    
    
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

    // make sure data is loaded
    w_rc_t rcl = _g_shore_env->loaddata();
    if (rcl.is_error()) {
        return (SHELL_NEXT_QUIT);
    }
    

    // create and start the dora-tpcc-db
    _g_dora = new dora_tpcc_db(_g_shore_env);
    assert (_g_dora);
    _g_dora->start();
    

    // start processing commands
    simple_shore_shell sss(_g_shore_env);
    sss.start();


    // stop and delete the dora-tpcc-db
    _g_dora->stop();
    delete (_g_dora);


    /* 3. Close the Shore environment */
    if (_g_shore_env)
        close_test_env();

    return (0);
}



