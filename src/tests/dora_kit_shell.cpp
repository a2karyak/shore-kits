/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_kit_shell.cpp
 *
 *  @brief:  Test shore/dora tpc-c kit with shell
 *
 *  @author: Ippokratis Pandis, Sept 2008
 *
 */

#include "tests/common.h"
#include "stages/tpcc/shore/shore_tpcc_env.h"
#include "sm/shore/shore_helper_loader.h"

#include "util/shell.h"

#include "dora.h"
#include "dora/tpcc/dora_tpcc.h"


using namespace shore;
using namespace tpcc;
using namespace dora;



//////////////////////////////


class dora_tpcc_kit_shell_t : public shore_kit_shell_t 
{ 
public:

    dora_tpcc_kit_shell_t(const char* prompt) 
        : shore_kit_shell_t(prompt, _g_shore_env)
        {
        }

    ~dora_tpcc_kit_shell_t() { }

    // impl of supported commands
    virtual int _cmd_TEST_impl(const int iQueriedWHs, const int iSpread,
                               const int iNumOfThreads, const int iNumOfTrxs,
                               const int iSelectedTrx, const int iIterations,
                               const int iUseSLI, const eBindingType abt);
    virtual int _cmd_MEASURE_impl(const int iQueriedWHs, const int iSpread,
                                  const int iNumOfThreads, const int iDuration,
                                  const int iSelectedTrx, const int iIterations,
                                  const int iUseSLI, const eBindingType abt);    

    virtual int process_cmd_LOAD(const char* command, char* command_tag);        

    virtual void append_trxs_map(void); // appends the DORA-TRXs

}; // EOF: dora_tpcc_kit_shell_t



/** dora_tpcc_kit_shell_t helper functions */

void dora_tpcc_kit_shell_t::append_trxs_map(void)
{
    // Baseline TPC-C trxs
    _sup_trxs[XCT_DORA_MIX]          = "DORA-Mix";
    _sup_trxs[XCT_DORA_NEW_ORDER]    = "DORA-NewOrder";
    _sup_trxs[XCT_DORA_PAYMENT]      = "DORA-Payment";
    _sup_trxs[XCT_DORA_ORDER_STATUS] = "DORA-OrderStatus";
    _sup_trxs[XCT_DORA_DELIVERY]     = "DORA-Delivery";
    _sup_trxs[XCT_DORA_STOCK_LEVEL]  = "DORA-StockLevel";

    // Microbenchmarks
    _sup_trxs[XCT_DORA_BENCH_WHS]   = "DORA-Bench-WHs";
    _sup_trxs[XCT_DORA_BENCH_CUST]  = "DORA-Bench-CUSTs";
}


/** cmd: TEST **/

int dora_tpcc_kit_shell_t::_cmd_TEST_impl(const int iQueriedWHs, 
                                          const int iSpread,
                                          const int iNumOfThreads, 
                                          const int iNumOfTrxs,
                                          const int iSelectedTrx, 
                                          const int iIterations,
                                          const int iUseSLI,
                                          const eBindingType abt)
{
    // print test information
    print_TEST_info(iQueriedWHs, iSpread, iNumOfThreads, 
                    iNumOfTrxs, iSelectedTrx, iIterations, iUseSLI, abt);

    client_smt_t* testers[MAX_NUM_OF_THR];
    for (int j=0; j<iIterations; j++) {

        TRACE( TRACE_ALWAYS, "Iteration [%d of %d]\n",
               (j+1), iIterations);

        // reset starting cpu and wh id
        _current_prs_id = _start_prs_id;
        int wh_id = 0;

        // set measurement state to measure - start counting everything
        _env->set_measure(MST_MEASURE);
	stopwatch_t timer;

        for (int i=0; i<iNumOfThreads; i++) {
            // create & fork testing threads
            if (iSpread)
                wh_id = i+1;
            testers[i] = new client_smt_t(_env, MT_NUM_OF_TRXS,
                                          wh_id, iSelectedTrx, 
                                          iNumOfTrxs, iUseSLI,
                                          c_str("dora-cl-%d", i),
                                          _current_prs_id);
            testers[i]->fork();
            _current_prs_id = next_cpu(abt, _current_prs_id);
        }

        /* 2. join the tester threads */
        for (int i=0; i<iNumOfThreads; i++) {
            testers[i]->join();
            if (testers[i]->_rv) {
                TRACE( TRACE_ALWAYS, "Error in testing...\n");
                TRACE( TRACE_ALWAYS, "Exiting...\n");
                assert (false);
            }    
            delete (testers[i]);
        }

	double delay = timer.time();

        // print throughput and reset session stats
        print_throughput(iQueriedWHs, iSpread, iNumOfThreads, iUseSLI, delay, abt);
        _env->reset_session_tpcc_stats();
    }

    // set measurement state
    _env->set_measure(MST_DONE);
    return (SHELL_NEXT_CONTINUE);
}



/** cmd: MEASURE **/

int dora_tpcc_kit_shell_t::_cmd_MEASURE_impl(const int iQueriedWHs, 
                                             const int iSpread,
                                             const int iNumOfThreads, 
                                             const int iDuration,
                                             const int iSelectedTrx, 
                                             const int iIterations,
                                             const int iUseSLI,
                                             const eBindingType abt)
{
    // print measurement info
    print_MEASURE_info(iQueriedWHs, iSpread, iNumOfThreads, iDuration, 
                       iSelectedTrx, iIterations, iUseSLI, abt);

    // create and fork client threads
    client_smt_t* testers[MAX_NUM_OF_THR];
    for (int j=0; j<iIterations; j++) {

        TRACE( TRACE_ALWAYS, "Iteration [%d of %d]\n",
               (j+1), iIterations);

        // reset starting cpu and wh id
        _current_prs_id = _start_prs_id;
        int wh_id = 0;

        // set measurement state
        _env->set_measure(MST_WARMUP);

        // create threads
        for (int i=0; i<iNumOfThreads; i++) {
            // create & fork testing threads
            if (iSpread)
                wh_id = i+1;
            testers[i] = new client_smt_t(_env, MT_TIME_DUR,
                                          wh_id, iSelectedTrx, 
                                          0, iUseSLI,
                                          c_str("dora-cl-%d", i),
                                          _current_prs_id);
            assert (testers[i]);
            testers[i]->fork();
            _current_prs_id = next_cpu(abt, _current_prs_id);
        }

        // TODO: give them some time (2secs) to start-up
        sleep(DF_WARMUP_INTERVAL);

        // set measurement state
        _env->set_measure(MST_MEASURE);
        alarm(iDuration);
	stopwatch_t timer;


        /* 2. join the tester threads */
        for (int i=0; i<iNumOfThreads; i++) {
            testers[i]->join();
            if (testers[i]->_rv) {
                TRACE( TRACE_ALWAYS, "Error in testing...\n");
                assert (false);
            }    
            assert (testers[i]);
            delete (testers[i]);
        }

	double delay = timer.time();
	alarm(0); // cancel the alarm, if any

        // print throughput and reset session stats
        print_throughput(iQueriedWHs, iSpread, iNumOfThreads, iUseSLI, delay, abt);
        _env->reset_session_tpcc_stats();
    }

    // set measurement state
    _env->set_measure(MST_DONE);
    return (SHELL_NEXT_CONTINUE);
}


int dora_tpcc_kit_shell_t::process_cmd_LOAD(const char* command, char* command_tag)
{
    TRACE( TRACE_DEBUG, "Gotcha\n");
    return (SHELL_NEXT_CONTINUE);
}


/** EOF: dora_tpcc_kit_shell_t functions */


//////////////////////////////


//////////////////////////////


int main(int argc, char* argv[]) 
{
    // initialize cordoba threads
    thread_init();

    TRACE_SET( TRACE_ALWAYS | TRACE_STATISTICS | TRACE_NETWORK | TRACE_CPU_BINDING
               //              | TRACE_QUERY_RESULTS
               //              | TRACE_PACKET_FLOW
               //               | TRACE_RECORD_FLOW
               | TRACE_TRX_FLOW
               | TRACE_DEBUG
              );

    /* 1. Instanciate the Shore environment */
    if (inst_test_env(argc, argv))
        return (1);

    /* 2. Make sure data is loaded */
    w_rc_t rcl = _g_shore_env->loaddata();
    if (rcl.is_error()) {
        return (SHELL_NEXT_QUIT);
    }

    /* 3. Create and start the dora-tpcc-db VAS */
    assert (_g_shore_env);
    _g_dora = new dora_tpcc_db(_g_shore_env);
    assert (_g_dora);
    _g_dora->start();

    /* 4. Start processing commands */
    dora_tpcc_kit_shell_t dora_kit_shell("(dora-tpcc-kit) ");
    dora_kit_shell.start();

    /* 5. stop and delete the dora-tpcc-db */
    _g_dora->stop();
    delete (_g_dora);

    /* 6. Close the Shore environment */
    if (_g_shore_env)
        close_test_env();

    return (0);
}


