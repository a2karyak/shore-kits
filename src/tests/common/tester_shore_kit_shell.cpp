/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   tester_shore_kit_shell.cpp
 *
 *  @brief:  Implementation of shell class for testing Shore environments
 *
 *  @author: Ippokratis Pandis (ipandis), Sept 2008
 *
 */

#include "tests/common/tester_shore_kit_shell.h"

using namespace shore;



extern "C" void alarm_handler(int sig) {
    _g_shore_env->set_measure(MST_DONE);
}

bool volatile _g_canceled = false;


/** shore_kit_shell_t interface */




/******************************************************************** 
 *
 *  @fn:    load_{trxs,bp}_map
 *
 *  @brief: Loads the basic supported trxs and binding policies maps
 *
 *  @note:  Calls the append_{trxs,bp}_map function for possible extensions
 *
 ********************************************************************/


// TRXS //

void shore_kit_shell_t::load_trxs_map(void)
{
    // Baseline TPC-C trxs
    _sup_trxs[XCT_MIX]          = "Mix";
    _sup_trxs[XCT_NEW_ORDER]    = "NewOrder";
    _sup_trxs[XCT_PAYMENT]      = "Payment";
    _sup_trxs[XCT_ORDER_STATUS] = "OrderStatus";
    _sup_trxs[XCT_DELIVERY]     = "Delivery";
    _sup_trxs[XCT_STOCK_LEVEL]  = "StockLevel";

    // Microbenchmarks
    _sup_trxs[XCT_MBENCH_WH]    = "MBench-WHs";
    _sup_trxs[XCT_MBENCH_CUST]  = "MBench-CUSTs";

    // Call virtual
    append_trxs_map();
}

void shore_kit_shell_t::print_sup_trxs(void) const 
{
    TRACE( TRACE_ALWAYS, "Supported TRXs\n");
    for (mapSupTrxsConstIt cit= _sup_trxs.begin();
         cit != _sup_trxs.end(); cit++)
            TRACE( TRACE_ALWAYS, "%d -> %s\n", cit->first, cit->second.c_str());
}

const char* shore_kit_shell_t::translate_trx(const int iSelectedTrx) const
{
    mapSupTrxsConstIt cit = _sup_trxs.find(iSelectedTrx);
    if (cit != _sup_trxs.end())
        return (cit->second.c_str());
    return ("Unsupported TRX");
}


// BP - Binding Policies //

void shore_kit_shell_t::load_bp_map(void)
{
    // Basic binding policies
    _sup_bps[BT_NONE]          = "NoBinding";
    _sup_bps[BT_NEXT]          = "Adjacent";
    _sup_bps[BT_SPREAD]        = "SpreadToCores";

    // Call virtual
    append_bp_map();
}

void shore_kit_shell_t::print_sup_bp(void) 
{
    TRACE( TRACE_ALWAYS, "Supported Binding Policies\n");
    for (mapBindPolsIt cit= _sup_bps.begin();
         cit != _sup_bps.end(); cit++)
            TRACE( TRACE_ALWAYS, "%d -> %s\n", cit->first, cit->second.c_str());
}

const char* shore_kit_shell_t::translate_bp(const eBindingType abt)
{
    mapBindPolsIt it = _sup_bps.find(abt);
    if (it != _sup_bps.end())
        return (it->second.c_str());
    return ("Unsupported Binding Policy");
}


/****************************************************************** 
 *
 * @fn:    next_cpu()
 *
 * @brief: Decides what is the next cpu for the forking clients 
 *
 * @note:  This decision is based on:
 *         - abt                     - the selected binding type
 *         - aprd                    - the current cpu
 *         - _env->_max_cpu_count    - the maximum cpu count (hard-limit)
 *         - _env->_active_cpu_count - the active cpu count (soft-limit)
 *         - this->_start_prs_id     - the first assigned cpu for the table
 *
 ******************************************************************/

processorid_t shore_kit_shell_t::next_cpu(const eBindingType abt, 
                                          const processorid_t aprd) 
{
    processorid_t nextprs;
    switch (abt) {
    case (BT_NONE):
        return (PBIND_NONE);
    case (BT_NEXT):
        nextprs = ((aprd+1) % _env->get_active_cpu_count());
        return (nextprs);
    case (BT_SPREAD):
        static const int NIAGARA_II_STEP = 8;
        nextprs = ((aprd+NIAGARA_II_STEP) % _env->get_active_cpu_count());
        return (nextprs);
    }
    assert (0); // Should not reach this point
    return (nextprs);
}


/******************************************************************** 
 *
 *  @fn:    print_throughput
 *
 *  @brief: Prints the throughput given measurement delay
 *
 ********************************************************************/

void shore_kit_shell_t::print_throughput(const int iQueriedWHs, const int iSpread, 
                                         const int iNumOfThreads, const int iUseSLI, 
                                         const double delay, const eBindingType abt)
{
    assert (_env);
    int trxs_att  = _env->get_session_tpcc_stats()->get_total_attempted();
    int trxs_com  = _env->get_session_tpcc_stats()->get_total_committed();
    int nords_com = _env->get_session_tpcc_stats()->get_no_com();

    TRACE( TRACE_ALWAYS, "*******\n"            \
           "WHs:      (%d)\n"                   \
           "Spread:   (%s)\n"                   \
           "SLI:      (%s)\n"                   \
           "Binding:  (%s)\n"                   \
           "Threads:  (%d)\n"                   \
           "Trxs Att: (%d)\n"                   \
           "Trxs Com: (%d)\n"                   \
           "NOrd Com: (%d)\n"       \
           "Secs:     (%.2f)\n"     \
           "TPS:      (%.2f)\n"                 \
           "tpm-C:    (%.2f)\n",
           iQueriedWHs, 
           (iSpread ? "Yes" : "No"), (iUseSLI ? "Yes" : "No"), 
           translate_bp(abt),
           iNumOfThreads, trxs_att, trxs_com, nords_com, 
           delay, 
           trxs_com/delay,
           60*nords_com/delay);
}

void shore_kit_shell_t::print_MEASURE_info(const int iQueriedWHs, const int iSpread, 
                                           const int iNumOfThreads, const int iDuration,
                                           const int iSelectedTrx, const int iIterations,
                                           const int iUseSLI, const eBindingType abt)
{
    // Print out configuration
    TRACE( TRACE_ALWAYS, "\n\n" \
           "Queried WHs    : %d\n" \
           "Spread Threads : %s\n" \
           "Binding        : %s\n" \
           "Num of Threads : %d\n" \
           "Duration       : %d\n" \
           "Trx            : %s\n" \
           "Iterations     : %d\n" \
           "Use SLI        : %s\n", 
           iQueriedWHs, (iSpread ? "Yes" : "No"), 
           translate_bp(abt),
           iNumOfThreads, iDuration, translate_trx(iSelectedTrx), 
           iIterations, (iUseSLI ? "Yes" : "No"));
}

void shore_kit_shell_t::print_TEST_info(const int iQueriedWHs, const int iSpread, 
                                        const int iNumOfThreads, const int iNumOfTrxs,
                                        const int iSelectedTrx, const int iIterations,
                                        const int iUseSLI, const eBindingType abt)
{
    // Print out configuration
    TRACE( TRACE_ALWAYS, "\n\n" \
           "Queried WHs    : %d\n" \
           "Spread Threads : %s\n" \
           "Binding        : %s\n" \
           "Num of Threads : %d\n" \
           "Num of Trxs    : %d\n" \
           "Trx            : %s\n" \
           "Iterations     : %d\n" \
           "Use SLI        : %s\n", 
           iQueriedWHs, (iSpread ? "Yes" : "No"), 
           translate_bp(abt),
           iNumOfThreads, iNumOfTrxs, translate_trx(iSelectedTrx),
           iIterations, (iUseSLI ? "Yes" : "No"));
}


/******************************************************************** 
 *
 *  @fn:    print_usage
 *
 *  @brief: Prints the normal usage for {TEST/MEASURE/WARMUP} cmds
 *
 ********************************************************************/

int shore_kit_shell_t::print_usage(const char* command) 
{
    assert (command);

    TRACE( TRACE_ALWAYS, "\n\nSupported commands: LOAD/WARMUP/TEST/MEASURE\n\n" );

    TRACE( TRACE_ALWAYS, "WARMUP Usage:\n\n" \
           "*** warmup [<NUM_QUERIED> <NUM_TRXS> <DURATION> <ITERATIONS>]\n" \
           "\nParameters:\n" \
           "<NUM_QUERIED> : The number of WHs queried (Default=10) (optional)\n" \
           "<NUM_TRXS>    : Number of transactions per thread (Default=1000) (optional)\n" \
           "<DURATION>    : Duration of experiment in secs (Default=20) (optional)\n" \
           "<ITERATIONS>  : Number of iterations (Default=3) (optional)\n\n");

    TRACE( TRACE_ALWAYS, "TEST Usage:\n\n" \
           "*** test <NUM_QUERIED> [<SPREAD> <NUM_THRS> <NUM_TRXS> <TRX_ID> <ITERATIONS> <SLI> <BINDING>]\n" \
           "\nParameters:\n" \
           "<NUM_QUERIED> : The number of WHs queried (queried factor)\n" \
           "<SPREAD>      : Whether to spread threads to WHs (0=No, Otherwise=Yes, Default=No) (optional)\n" \
           "<NUM_THRS>    : Number of threads used (optional)\n" \
           "<NUM_TRXS>    : Number of transactions per thread (optional)\n" \
           "<TRX_ID>      : Transaction ID to be executed (0=mix) (optional)\n" \
           "<ITERATIONS>  : Number of iterations (Default=5) (optional)\n" \
           "<SLI>         : Use SLI (Default=0-No) (optional)\n" \
           "<BINDING>     : Binding Type (Default=0-No binding) (optional)\n\n");

    TRACE( TRACE_ALWAYS, "MEASURE Usage:\n\n" \
           "*** measure <NUM_QUERIED> [<SPREAD> <NUM_THRS> <DURATION> <TRX_ID> <ITERATIONS> <SLI> <BINDING>]\n" \
           "\nParameters:\n" \
           "<NUM_QUERIED> : The number of WHs queried (queried factor)\n" \
           "<SPREAD>      : Whether to spread threads to WHs (0=No, Otherwise=Yes, Default=No) (optional)\n" \
           "<NUM_THRS>    : Number of threads used (optional)\n" \
           "<DURATION>    : Duration of experiment in secs (Default=20) (optional)\n" \
           "<TRX_ID>      : Transaction ID to be executed (0=mix) (optional)\n" \
           "<ITERATIONS>  : Number of iterations (Default=5) (optional)\n" \
           "<SLI>         : Use SLI (Default=0-No) (optional)\n" \
           "<BINDING>     : Binding Type (Default=0-No binding) (optional)\n");
    
    TRACE( TRACE_ALWAYS, "\n\nCurrently numOfWHs = (%d)\n", _numOfWHs);

    print_sup_trxs();
    print_sup_bp();

    return (SHELL_NEXT_CONTINUE);
}


/******************************************************************** 
 *
 *  @fn:    usage_cmd_TEST
 *
 *  @brief: Prints the usage for TEST cmd
 *
 ********************************************************************/

void shore_kit_shell_t::usage_cmd_TEST() 
{
    TRACE( TRACE_ALWAYS, "TEST Usage:\n\n" \
           "*** test <NUM_QUERIED> [<SPREAD> <NUM_THRS> <NUM_TRXS> <TRX_ID> <ITERATIONS> <SLI> <BINDING>]\n" \
           "\nParameters:\n" \
           "<NUM_QUERIED> : The number of WHs queried (queried factor)\n" \
           "<SPREAD>      : Whether to spread threads to WHs (0=No, Otherwise=Yes, Default=No) (optional)\n" \
           "<NUM_THRS>    : Number of threads used (optional)\n" \
           "<NUM_TRXS>    : Number of transactions per thread (optional)\n" \
           "<TRX_ID>      : Transaction ID to be executed (0=mix) (optional)\n" \
           "<ITERATIONS>  : Number of iterations (Default=5) (optional)\n" \
           "<SLI>         : Use SLI (Default=0-No) (optional)\n" \
           "<BINDING>     : Binding Type (Default=0-No binding) (optional)\n\n");
}


/******************************************************************** 
 *
 *  @fn:    usage_cmd_MEASURE
 *
 *  @brief: Prints the usage for MEASURE cmd
 *
 ********************************************************************/

void shore_kit_shell_t::usage_cmd_MEASURE() 
{
    TRACE( TRACE_ALWAYS, "MEASURE Usage:\n\n"                           \
           "*** measure <NUM_QUERIED> [<SPREAD> <NUM_THRS> <DURATION> <TRX_ID> <ITERATIONS> <SLI> <BINDING>]\n" \
           "\nParameters:\n" \
           "<NUM_QUERIED> : The number of WHs queried (queried factor)\n" \
           "<SPREAD>      : Whether to spread threads to WHs (0=No, Otherwise=Yes, Default=No) (optional)\n" \
           "<NUM_THRS>    : Number of threads used (optional)\n" \
           "<DURATION>    : Duration of experiment in secs (Default=20) (optional)\n" \
           "<TRX_ID>      : Transaction ID to be executed (0=mix) (optional)\n" \
           "<ITERATIONS>  : Number of iterations (Default=5) (optional)\n" \
           "<SLI>         : Use SLI (Default=0-No) (optional)\n" \
           "<BINDING>     : Binding Type (Default=0-No binding) (optional)\n\n");
}


/******************************************************************** 
 *
 *  @fn:    usage_cmd_WARMUP
 *
 *  @brief: Prints the usage for WARMUP cmd
 *
 ********************************************************************/

void shore_kit_shell_t::usage_cmd_WARMUP() 
{
    TRACE( TRACE_ALWAYS, "WARMUP Usage:\n\n" \
           "*** warmup [<NUM_QUERIED> <NUM_TRXS> <DURATION> <ITERATIONS>]\n" \
           "\nParameters:\n" \
           "<NUM_QUERIED> : The number of WHs queried (Default=10) (optional)\n" \
           "<NUM_TRXS>    : Number of transactions per thread (Default=1000) (optional)\n" \
           "<DURATION>    : Duration of experiment in secs (Default=20) (optional)\n" \
           "<ITERATIONS>  : Number of iterations (Default=3) (optional)\n\n");
}


/******************************************************************** 
 *
 *  @fn:    usage_cmd_LOAD
 *
 *  @brief: Prints the usage for LOAD cmd
 *
 ********************************************************************/

void shore_kit_shell_t::usage_cmd_LOAD() 
{
    TRACE( TRACE_ALWAYS, "LOAD Usage:\n\n" \
           "*** load\n");
}



/******************************************************************** 
 *
 *  @fn:    process_command
 *
 *  @brief: Catches the {TEST/MEASURE/WARMUP} cmds
 *
 ********************************************************************/

int shore_kit_shell_t::process_command(const char* command)
{
    _g_canceled = false;

    _current_prs_id = _start_prs_id;

    char command_tag[SERVER_COMMAND_BUFFER_SIZE];

    // make sure any previous abort is cleared
    client_smt_t::resume_test();

    if ( sscanf(command, "%s", &command_tag) < 1) {
        print_usage(command_tag);
        return (SHELL_NEXT_CONTINUE);
    }

    // LOAD cmd
    if (strcasecmp(command_tag, "LOAD") == 0) {
        return (process_cmd_LOAD(command, command_tag));
    }

    // WARMUP cmd
    if (strcasecmp(command_tag, "WARMUP") == 0) {
        return (process_cmd_WARMUP(command, command_tag));
    }
    
    // TEST cmd
    if (strcasecmp(command_tag, "TEST") == 0) {
        return (process_cmd_TEST(command, command_tag));
    }

    // MEASURE cmd
    if (strcasecmp(command_tag, "MEASURE") == 0) {
        return (process_cmd_MEASURE(command, command_tag));
    }
    else {
        print_usage(command_tag);
        return (SHELL_NEXT_CONTINUE);
    }        

    assert (0); // should never reach this point
    return (SHELL_NEXT_CONTINUE);
}



/******************************************************************** 
 *
 *  @fn:    process_cmd_WARMUP
 *
 *  @brief: Parses the WARMUP cmd and calls the virtual impl function
 *
 ********************************************************************/

int shore_kit_shell_t::process_cmd_WARMUP(const char* command, 
                                          char* command_tag)
{
    assert (_env);
    assert (_env->is_initialized());

    // first check if env initialized and loaded
    // try to load and abort on error
    w_rc_t rcl = _env->loaddata();
    if (rcl.is_error()) {
        return (SHELL_NEXT_QUIT);
    }

    /* 0. Parse Parameters */
    int numOfQueriedWHs     = DF_WARMUP_QUERIED_WHS;
    int tmp_numOfQueriedWHs = DF_WARMUP_QUERIED_WHS;
    int numOfTrxs           = DF_WARMUP_TRX_PER_THR;
    int tmp_numOfTrxs       = DF_WARMUP_TRX_PER_THR;
    int duration            = DF_WARMUP_DURATION;
    int tmp_duration        = DF_WARMUP_DURATION;
    int iterations          = DF_WARMUP_ITERS;
    int tmp_iterations      = DF_WARMUP_ITERS;
    
    // Parses new test run data
    if ( sscanf(command, "%s %d %d %d %d",
                &command_tag,
                &tmp_numOfQueriedWHs,
                &tmp_numOfTrxs,
                &tmp_duration,
                &tmp_iterations) < 1 ) 
    {
        usage_cmd_WARMUP();
        return (SHELL_NEXT_CONTINUE);
    }


    // OPTIONAL Parameters

    // 1- number of queried warehouses - numOfQueriedWHs
    if ((tmp_numOfQueriedWHs>0) && (tmp_numOfQueriedWHs<=_numOfWHs))
        numOfQueriedWHs = tmp_numOfQueriedWHs;
    else
        numOfQueriedWHs = _numOfWHs;
    assert (numOfQueriedWHs <= _numOfWHs);
    
    // 2- number of trxs - numOfTrxs
    if (tmp_numOfTrxs>0)
        numOfTrxs = tmp_numOfTrxs;
    
    // 3- duration of measurement - duration
    if (tmp_duration>0)
        duration = tmp_duration;

    // 4- number of iterations - iterations
    if (tmp_iterations>0)
        iterations = tmp_iterations;


    // Print out configuration
    TRACE( TRACE_ALWAYS, "\n\n" \
           "Queried WHs    : %d\n" \
           "Num of Trxs    : %d\n" \
           "Duration       : %d\n" \
           "Iterations     : %d\n", 
           numOfQueriedWHs, numOfTrxs, duration, iterations);

    // call the virtual function that implements the test    
    return (_cmd_WARMUP_impl(numOfQueriedWHs, numOfTrxs, duration, iterations));
}



/******************************************************************** 
 *
 *  @fn:    process_cmd_LOAD
 *
 *  @brief: Parses the LOAD cmd and calls the virtual impl function
 *
 ********************************************************************/

int shore_kit_shell_t::process_cmd_LOAD(const char* command, 
                                        char* command_tag)
{
    assert (_env);
    assert (_env->is_initialized());

    if (_env->is_loaded()) {
        TRACE( TRACE_ALWAYS, "Environment already loaded\n");
        return (SHELL_NEXT_CONTINUE);
    }

    // call the virtual function that implements the test    
    return (_cmd_LOAD_impl());
}


/******************************************************************** 
 *
 *  @fn:    process_cmd_TEST
 *
 *  @brief: Parses the TEST cmd and calls the virtual impl function
 *
 ********************************************************************/

int shore_kit_shell_t::process_cmd_TEST(const char* command, 
                                        char* command_tag)
{
    assert (_env);
    assert (_env->is_initialized());

    // first check if env initialized and loaded
    // try to load and abort on error
    w_rc_t rcl = _env->loaddata();
    if (rcl.is_error()) {
        return (SHELL_NEXT_QUIT);
    }


    /* 0. Parse Parameters */
    int numOfQueriedWHs      = DF_NUM_OF_QUERIED_WHS;
    int tmp_numOfQueriedWHs  = DF_NUM_OF_QUERIED_WHS;
    int spreadThreads        = DF_SPREAD_THREADS_TO_WHS;
    int tmp_spreadThreads    = DF_SPREAD_THREADS_TO_WHS;
    int numOfThreads         = DF_NUM_OF_THR;
    int tmp_numOfThreads     = DF_NUM_OF_THR;
    int numOfTrxs            = DF_TRX_PER_THR;
    int tmp_numOfTrxs        = DF_TRX_PER_THR;
    int selectedTrxID        = DF_TRX_ID;
    int tmp_selectedTrxID    = DF_TRX_ID;
    int iterations           = DF_NUM_OF_ITERS;
    int tmp_iterations       = DF_NUM_OF_ITERS;
    int use_sli              = DF_USE_SLI;
    int tmp_use_sli          = DF_USE_SLI;
    eBindingType binding     = DF_BINDING_TYPE;
    eBindingType tmp_binding = DF_BINDING_TYPE;
    
    // Parses new test run data
    if ( sscanf(command, "%s %d %d %d %d %d %d %d %d",
                &command_tag,
                &tmp_numOfQueriedWHs,
                &tmp_spreadThreads,
                &tmp_numOfThreads,
                &tmp_numOfTrxs,
                &tmp_selectedTrxID,
                &tmp_iterations,
                &tmp_use_sli,
                &tmp_binding) < 2 ) 
    {
        usage_cmd_TEST();
        return (SHELL_NEXT_CONTINUE);
    }


    // REQUIRED Parameters

    // 1- number of queried warehouses - numOfQueriedWHs
    if ((tmp_numOfQueriedWHs>0) && (tmp_numOfQueriedWHs<=_numOfWHs))
        numOfQueriedWHs = tmp_numOfQueriedWHs;
    else
        numOfQueriedWHs = _numOfWHs;
    assert (numOfQueriedWHs <= _numOfWHs);


    // OPTIONAL Parameters

    // 2- spread trxs
    spreadThreads = tmp_spreadThreads;

    // 3- number of threads - numOfThreads
    if ((tmp_numOfThreads>0) && (tmp_numOfThreads<=MAX_NUM_OF_THR)) {
        numOfThreads = tmp_numOfThreads;
        if (spreadThreads && (numOfThreads > numOfQueriedWHs))
            numOfThreads = numOfQueriedWHs;
    }
    else {
        numOfThreads = numOfQueriedWHs;
    }
    
    // 4- number of trxs - numOfTrxs
    if (tmp_numOfTrxs>0)
        numOfTrxs = tmp_numOfTrxs;

    // 5- selected trx
    if (tmp_selectedTrxID>=0)
        selectedTrxID = tmp_selectedTrxID;

    // 6- number of iterations - iterations
    if (tmp_iterations>0)
        iterations = tmp_iterations;

    // 7- use sli   
    use_sli = tmp_use_sli;

    // 8- binding type   
    if (tmp_binding>BT_NONE) {        
        mapBindPolsIt cit = _sup_bps.find(tmp_binding);
        if (cit!= _sup_bps.end())
            binding = tmp_binding;
    }

    // call the virtual function that implements the test    
    return (_cmd_TEST_impl(numOfQueriedWHs, spreadThreads, numOfThreads,
                           numOfTrxs, selectedTrxID, iterations, use_sli, 
                           binding));
}



/******************************************************************** 
 *
 *  @fn:    process_cmd_MEASURE
 *
 *  @brief: Parses the MEASURE cmd and calls the virtual impl function
 *
 ********************************************************************/

int shore_kit_shell_t::process_cmd_MEASURE(const char* command, 
                                           char* command_tag)
{
    assert (_env);
    assert (_env->is_initialized());

    // first check if env initialized and loaded
    // try to load and abort on error
    w_rc_t rcl = _env->loaddata();
    if (rcl.is_error()) {
        return (SHELL_NEXT_QUIT);
    }

    TRACE( TRACE_ALWAYS, "measuring...\n");

    /* 0. Parse Parameters */
    int numOfQueriedWHs      = DF_NUM_OF_QUERIED_WHS;
    int tmp_numOfQueriedWHs  = DF_NUM_OF_QUERIED_WHS;
    int spreadThreads        = DF_SPREAD_THREADS_TO_WHS;
    int tmp_spreadThreads    = DF_SPREAD_THREADS_TO_WHS;
    int numOfThreads         = DF_NUM_OF_THR;
    int tmp_numOfThreads     = DF_NUM_OF_THR;
    int duration             = DF_DURATION;
    int tmp_duration         = DF_DURATION;
    int selectedTrxID        = DF_TRX_ID;
    int tmp_selectedTrxID    = DF_TRX_ID;
    int iterations           = DF_NUM_OF_ITERS;
    int tmp_iterations       = DF_NUM_OF_ITERS;
    int use_sli              = DF_USE_SLI;
    int tmp_use_sli          = DF_USE_SLI;
    eBindingType binding     = DF_BINDING_TYPE;
    eBindingType tmp_binding = DF_BINDING_TYPE;
    
    // Parses new test run data
    if ( sscanf(command, "%s %d %d %d %d %d %d %d %d",
                &command_tag,
                &tmp_numOfQueriedWHs,
                &tmp_spreadThreads,
                &tmp_numOfThreads,
                &tmp_duration,
                &tmp_selectedTrxID,
                &tmp_iterations,
                &tmp_use_sli,
                &tmp_binding) < 2 ) 
    {
        usage_cmd_MEASURE();
        return (SHELL_NEXT_CONTINUE);
    }


    // REQUIRED Parameters

    // 1- number of queried warehouses - numOfQueriedWHs
    if ((tmp_numOfQueriedWHs>0) && (tmp_numOfQueriedWHs<=_numOfWHs))
        numOfQueriedWHs = tmp_numOfQueriedWHs;
    else
        numOfQueriedWHs = _numOfWHs;
    assert (numOfQueriedWHs <= _numOfWHs);


    // OPTIONAL Parameters

    // 2- spread trxs
    spreadThreads = tmp_spreadThreads;

    // 3- number of threads - numOfThreads
    if ((tmp_numOfThreads>0) && (tmp_numOfThreads<=MAX_NUM_OF_THR)) {
        numOfThreads = tmp_numOfThreads;
        if (spreadThreads && (numOfThreads > numOfQueriedWHs))
            numOfThreads = numOfQueriedWHs;
    }
    else {
        numOfThreads = numOfQueriedWHs;
    }
    
    // 4- duration of measurement - duration
    if (tmp_duration>0)
        duration = tmp_duration;

    // 5- selected trx
    if (tmp_selectedTrxID>=0)
        selectedTrxID = tmp_selectedTrxID;

    // 6- number of iterations - iterations
    if (tmp_iterations>0)
        iterations = tmp_iterations;

    // 7- use sli   
    use_sli = tmp_use_sli;

    // 8- binding type   
    if (tmp_binding>BT_NONE)
        binding = tmp_binding;

    // call the virtual function that implements the measurement    
    return (_cmd_MEASURE_impl(numOfQueriedWHs, spreadThreads, numOfThreads,
                              duration, selectedTrxID, iterations, use_sli,
                              binding));
}


/******************************************************************** 
 *
 *  @fn:    SIGINT_handler
 *
 *  @brief: Aborts test/measurement
 *
 ********************************************************************/

int shore_kit_shell_t::SIGINT_handler() 
{
    if(_processing_command && !_g_canceled) {
	_g_canceled = true;
	client_smt_t::abort_test();
	return 0;
    }

    // fallback...
    return (shell_t::SIGINT_handler());
}


/******************************************************************** 
 *
 *  @fn:    _cmd_WARMUP_impl
 *
 *  @brief: Implementation of the WARMUP cmd
 *
 ********************************************************************/

int shore_kit_shell_t::_cmd_WARMUP_impl(const int iQueriedWHs, 
                                        const int iTrxs, 
                                        const int iDuration, 
                                        const int iIterations)
{
    TRACE( TRACE_ALWAYS, "warming up...\n");

    assert (_env);
    assert (_env->is_initialized());
    assert (_env->is_loaded());

    // if warmup fails abort
    w_rc_t rcw = _env->warmup();            
    if (rcw.is_error()) {
        assert (0); // should not fail
        return (SHELL_NEXT_QUIT);
    }
    return (SHELL_NEXT_CONTINUE);            
}


/******************************************************************** 
 *
 *  @fn:    _cmd_LOAD_impl
 *
 *  @brief: Implementation of the LOAD cmd
 *
 ********************************************************************/

int shore_kit_shell_t::_cmd_LOAD_impl()
{
    TRACE( TRACE_ALWAYS, "loading...\n");

    assert (_env);
    assert (_env->is_initialized());

    w_rc_t rcl = _env->loaddata();
    if (rcl.is_error()) {
        TRACE( TRACE_ALWAYS, "Problem loading data\n");
        return (SHELL_NEXT_QUIT);
    }
    assert (_env->is_loaded());
    return (SHELL_NEXT_CONTINUE);            
}



/** EOF: shore_kit_shell_t functions */
