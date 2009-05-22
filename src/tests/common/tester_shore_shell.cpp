/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   tester_shore_shell.cpp
 *
 *  @brief:  Implementation of shell class for testing Shore environments
 *
 *  @author: Ippokratis Pandis, Sept 2008
 */

#include "tests/common/tester_shore_shell.h"
#include "tests/common/tester_shore_client.h"

using namespace shore;


extern "C" void alarm_handler(int sig) {
    _g_shore_env->set_measure(MST_DONE);
}

bool volatile _g_canceled = false;


//// shore_shell_t interface ////


/******************************************************************** 
 *
 *  @fn:    {trxs,bp}_map
 *
 *  @brief: The supported trxs and binding policies maps
 *
 ********************************************************************/


// TRXS - Supported Transactions //

void shore_shell_t::print_sup_trxs(void) const 
{
    TRACE( TRACE_ALWAYS, "Supported TRXs\n");
    for (mapSupTrxsConstIt cit= _sup_trxs.begin();
         cit != _sup_trxs.end(); cit++)
            TRACE( TRACE_ALWAYS, "%d -> %s\n", cit->first, cit->second.c_str());
}

const char* shore_shell_t::translate_trx(const int iSelectedTrx) const
{
    mapSupTrxsConstIt cit = _sup_trxs.find(iSelectedTrx);
    if (cit != _sup_trxs.end())
        return (cit->second.c_str());
    return ("Unsupported TRX");
}


// BP - Binding Policies //

void shore_shell_t::print_sup_bp(void) 
{
    TRACE( TRACE_ALWAYS, "Supported Binding Policies\n");
    for (mapBindPolsIt cit= _sup_bps.begin();
         cit != _sup_bps.end(); cit++)
            TRACE( TRACE_ALWAYS, "%d -> %s\n", cit->first, cit->second.c_str());
}

const char* shore_shell_t::translate_bp(const eBindingType abt)
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

processorid_t shore_shell_t::next_cpu(const eBindingType abt, 
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
 *  @fn:    print_CMD_info
 *
 *  @brief: Prints command-specific info
 *
 ********************************************************************/

void shore_shell_t::print_MEASURE_info(const int iQueriedSF, const int iSpread, 
                                           const int iNumOfThreads, const int iDuration,
                                           const int iSelectedTrx, const int iIterations,
                                           const eBindingType abt)
{
    // Print out configuration
    TRACE( TRACE_ALWAYS, "\n\n" \
           "QueriedSF:     (%d)\n" \
           "SpreadThreads: (%s)\n" \
           "Binding:       (%s)\n" \
           "NumOfThreads:  (%d)\n" \
           "Duration:      (%d)\n" \
           "Trx:           (%s)\n" \
           "Iterations:    (%d)\n",
           iQueriedSF, (iSpread ? "Yes" : "No"), 
           translate_bp(abt),
           iNumOfThreads, iDuration, translate_trx(iSelectedTrx), 
           iIterations);
}

void shore_shell_t::print_TEST_info(const int iQueriedSF, const int iSpread, 
                                        const int iNumOfThreads, const int iNumOfTrxs,
                                        const int iSelectedTrx, const int iIterations,
                                        const eBindingType abt)
{
    // Print out configuration
    TRACE( TRACE_ALWAYS, "\n\n"
           "QueriedSF:      (%d)\n" \
           "Spread Threads: (%s)\n" \
           "Binding:        (%s)\n" \
           "NumOfThreads:   (%d)\n" \
           "NumOfTrxs:      (%d)\n" \
           "Trx:            (%s)\n" \
           "Iterations:     (%d)\n",
           iQueriedSF, (iSpread ? "Yes" : "No"), 
           translate_bp(abt),
           iNumOfThreads, iNumOfTrxs, translate_trx(iSelectedTrx),
           iIterations);
}


/******************************************************************** 
 *
 *  @fn:    print_usage
 *
 *  @brief: Prints the normal usage for {TEST/MEASURE/WARMUP} cmds
 *
 ********************************************************************/

int shore_shell_t::print_usage(const char* command) 
{
    assert (command);

    TRACE( TRACE_ALWAYS, "\n\nSupported commands: TRXS/LOAD/WARMUP/TEST/MEASURE\n\n" );

    TRACE( TRACE_ALWAYS, "WARMUP Usage:\n\n" \
           "*** warmup [<NUM_QUERIED> <NUM_TRXS> <DURATION> <ITERATIONS>]\n" \
           "\nParameters:\n" \
           "<NUM_QUERIED> : The SF queried (Default=10) (optional)\n" \
           "<NUM_TRXS>    : Number of transactions per thread (Default=1000) (optional)\n" \
           "<DURATION>    : Duration of experiment in secs (Default=20) (optional)\n" \
           "<ITERATIONS>  : Number of iterations (Default=3) (optional)\n\n");

    TRACE( TRACE_ALWAYS, "TEST Usage:\n\n" \
           "*** test <NUM_QUERIED> [<SPREAD> <NUM_THRS> <NUM_TRXS> <TRX_ID> <ITERATIONS> <BINDING>]\n" \
           "\nParameters:\n" \
           "<NUM_QUERIED> : The SF queried (queried factor)\n" \
           "<SPREAD>      : Whether to spread threads (0=No, Other=Yes, Default=No) (optional)\n" \
           "<NUM_THRS>    : Number of threads used (optional)\n" \
           "<NUM_TRXS>    : Number of transactions per thread (optional)\n" \
           "<TRX_ID>      : Transaction ID to be executed (0=mix) (optional)\n" \
           "<ITERATIONS>  : Number of iterations (Default=5) (optional)\n" \
           "<BINDING>     : Binding Type (Default=0-No binding) (optional)\n\n");

    TRACE( TRACE_ALWAYS, "MEASURE Usage:\n\n" \
           "*** measure <NUM_QUERIED> [<SPREAD> <NUM_THRS> <DURATION> <TRX_ID> <ITERATIONS> <BINDING>]\n" \
           "\nParameters:\n" \
           "<NUM_QUERIED> : The SF queried (queried factor)\n" \
           "<SPREAD>      : Whether to spread threads (0=No, Other=Yes, Default=No) (optional)\n" \
           "<NUM_THRS>    : Number of threads used (optional)\n" \
           "<DURATION>    : Duration of experiment in secs (Default=20) (optional)\n" \
           "<TRX_ID>      : Transaction ID to be executed (0=mix) (optional)\n" \
           "<ITERATIONS>  : Number of iterations (Default=5) (optional)\n" \
           "<BINDING>     : Binding Type (Default=0-No binding) (optional)\n");
    
    TRACE( TRACE_ALWAYS, "\n\nCurrently Scaling factor = (%d)\n", _theSF);

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

void shore_shell_t::usage_cmd_TEST() 
{
    TRACE( TRACE_ALWAYS, "TEST Usage:\n\n" \
           "*** test <NUM_QUERIED> [<SPREAD> <NUM_THRS> <NUM_TRXS> <TRX_ID> <ITERATIONS> <BINDING>]\n" \
           "\nParameters:\n" \
           "<NUM_QUERIED> : The SF queried (queried factor)\n" \
           "<SPREAD>      : Whether to spread threads (0=No, Other=Yes, Default=No) (optional)\n" \
           "<NUM_THRS>    : Number of threads used (optional)\n" \
           "<NUM_TRXS>    : Number of transactions per thread (optional)\n" \
           "<TRX_ID>      : Transaction ID to be executed (0=mix) (optional)\n" \
           "<ITERATIONS>  : Number of iterations (Default=5) (optional)\n" \
           "<BINDING>     : Binding Type (Default=0-No binding) (optional)\n\n");
}


/******************************************************************** 
 *
 *  @fn:    usage_cmd_MEASURE
 *
 *  @brief: Prints the usage for MEASURE cmd
 *
 ********************************************************************/

void shore_shell_t::usage_cmd_MEASURE() 
{
    TRACE( TRACE_ALWAYS, "MEASURE Usage:\n\n"                           \
           "*** measure <NUM_QUERIED> [<SPREAD> <NUM_THRS> <DURATION> <TRX_ID> <ITERATIONS> <BINDING>]\n" \
           "\nParameters:\n" \
           "<NUM_QUERIED> : The SF queried (queried factor)\n" \
           "<SPREAD>      : Whether to spread threads (0=No, Other=Yes, Default=No) (optional)\n" \
           "<NUM_THRS>    : Number of threads used (optional)\n" \
           "<DURATION>    : Duration of experiment in secs (Default=20) (optional)\n" \
           "<TRX_ID>      : Transaction ID to be executed (0=mix) (optional)\n" \
           "<ITERATIONS>  : Number of iterations (Default=5) (optional)\n" \
           "<BINDING>     : Binding Type (Default=0-No binding) (optional)\n\n");
}


/******************************************************************** 
 *
 *  @fn:    usage_cmd_WARMUP
 *
 *  @brief: Prints the usage for WARMUP cmd
 *
 ********************************************************************/

void shore_shell_t::usage_cmd_WARMUP() 
{
    TRACE( TRACE_ALWAYS, "WARMUP Usage:\n\n" \
           "*** warmup [<NUM_QUERIED> <NUM_TRXS> <DURATION> <ITERATIONS>]\n" \
           "\nParameters:\n" \
           "<NUM_QUERIED> : The SF queried (Default=10) (optional)\n" \
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

void shore_shell_t::usage_cmd_LOAD() 
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

int shore_shell_t::process_command(const char* cmd,
                                   const char* cmd_tag)
{
    _g_canceled = false;

    _current_prs_id = _start_prs_id;

    // make sure any previous abort is cleared
    base_client_t::resume_test();


    // print processor usage info
    _cpustater->myinfo.reset();

    // TRXS cmd
    if (strcasecmp(cmd_tag, "TRXS") == 0) {
        return (process_cmd_TRXS(cmd, cmd_tag));
    }

    // LOAD cmd
    if (strcasecmp(cmd_tag, "LOAD") == 0) {
        return (process_cmd_LOAD(cmd, cmd_tag));
    }

    // WARMUP cmd
    if (strcasecmp(cmd_tag, "WARMUP") == 0) {
        return (process_cmd_WARMUP(cmd, cmd_tag));
    }
    
    // TEST cmd
    if (strcasecmp(cmd_tag, "TEST") == 0) {
        return (process_cmd_TEST(cmd, cmd_tag));
    }

    // MEASURE cmd
    if (strcasecmp(cmd_tag, "MEASURE") == 0) {
        return (process_cmd_MEASURE(cmd, cmd_tag));
    }
    else {
        TRACE( TRACE_ALWAYS, "Unknown command (%s)\n", cmd);
        //print_usage(cmd_tag);
        return (SHELL_NEXT_CONTINUE);
    }        

    assert (0); // should never reach this point
    return (SHELL_NEXT_CONTINUE);
}



/******************************************************************** 
 *
 *  @fn:    process_cmd_TRXS
 *
 *  @brief: Parses the TRXS cmd and calls the virtual impl function
 *
 ********************************************************************/

int shore_shell_t::process_cmd_TRXS(const char* command, 
                                    const char* command_tag)
{
    print_sup_trxs();
    return (SHELL_NEXT_CONTINUE);
}


/******************************************************************** 
 *
 *  @fn:    process_cmd_LOAD
 *
 *  @brief: Parses the LOAD cmd and calls the virtual impl function
 *
 ********************************************************************/

int shore_shell_t::process_cmd_LOAD(const char* command, 
                                    const char* command_tag)
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
 *  @fn:    process_cmd_WARMUP
 *
 *  @brief: Parses the WARMUP cmd and calls the virtual impl function
 *
 ********************************************************************/

int shore_shell_t::process_cmd_WARMUP(const char* command, 
                                      const char* command_tag)
{
    assert (_env);
    assert (_env->is_initialized());

    // first check if env initialized and loaded
    // try to load and abort on error
    w_rc_t rcl = _env->loaddata();
    if (rcl.is_error()) {
        return (SHELL_NEXT_QUIT);
    }

    // 0. Parse Parameters
    envVar* ev = envVar::instance();
    int numOfQueriedSF      = ev->getVarInt("test-num-queried",DF_WARMUP_QUERIED_SF);
    int tmp_numOfQueriedSF  = numOfQueriedSF;
    int numOfTrxs           = ev->getVarInt("test-num-trxs",DF_WARMUP_TRX_PER_THR);
    int tmp_numOfTrxs       = numOfTrxs;
    int duration            = ev->getVarInt("measure-duration",DF_WARMUP_DURATION);
    int tmp_duration        = duration;
    int iterations          = ev->getVarInt("test-iterations",DF_WARMUP_ITERS);
    int tmp_iterations      = iterations;


    
    // Parses new test run data
    if ( sscanf(command, "%s %d %d %d %d",
                &command_tag,
                &tmp_numOfQueriedSF,
                &tmp_numOfTrxs,
                &tmp_duration,
                &tmp_iterations) < 1 ) 
    {
        usage_cmd_WARMUP();
        return (SHELL_NEXT_CONTINUE);
    }


    // OPTIONAL Parameters

    // 1- number of queried scaling factor - numOfQueriedSF
    if ((tmp_numOfQueriedSF>0) && (tmp_numOfQueriedSF<=_theSF))
        numOfQueriedSF = tmp_numOfQueriedSF;
    else
        numOfQueriedSF = _theSF;
    assert (numOfQueriedSF <= _theSF);
    
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
           "Queried SF   : %d\n" \
           "Num of Trxs  : %d\n" \
           "Duration     : %d\n" \
           "Iterations   : %d\n", 
           numOfQueriedSF, numOfTrxs, duration, iterations);

    // call the virtual function that implements the test    
    return (_cmd_WARMUP_impl(numOfQueriedSF, numOfTrxs, duration, iterations));
}




/******************************************************************** 
 *
 *  @fn:    process_cmd_TEST
 *
 *  @brief: Parses the TEST cmd and calls the virtual impl function
 *
 ********************************************************************/

int shore_shell_t::process_cmd_TEST(const char* command, 
                                    const char* command_tag)
{
    assert (_env);
    assert (_env->is_initialized());

    // first check if env initialized and loaded
    // try to load and abort on error
    w_rc_t rcl = _env->loaddata();
    if (rcl.is_error()) {
        return (SHELL_NEXT_QUIT);
    }

    // 0. Parse Parameters
    envVar* ev = envVar::instance();
    int numOfQueriedSF      = ev->getVarInt("test-num-queried",DF_NUM_OF_QUERIED_SF);
    int tmp_numOfQueriedSF  = numOfQueriedSF;
    int spreadThreads        = ev->getVarInt("test-spread",DF_SPREAD_THREADS);
    int tmp_spreadThreads    = spreadThreads;
    int numOfThreads         = ev->getVarInt("test-num-threads",DF_NUM_OF_THR);
    int tmp_numOfThreads     = numOfThreads;
    int numOfTrxs            = ev->getVarInt("test-num-trxs",DF_TRX_PER_THR);
    int tmp_numOfTrxs        = numOfTrxs;
    int selectedTrxID        = ev->getVarInt("test-trx-id",DF_TRX_ID);
    int tmp_selectedTrxID    = selectedTrxID;
    int iterations           = ev->getVarInt("test-iterations",DF_NUM_OF_ITERS);
    int tmp_iterations       = iterations;
    eBindingType binding     = DF_BINDING_TYPE;//ev->getVarInt("test-cl-binding",DF_BINDING_TYPE);
    eBindingType tmp_binding = binding;


    // update the SF
    int tmp_sf = ev->getSysVarInt("sf");
    if (tmp_sf) {
        TRACE( TRACE_STATISTICS, "Updated SF (%d)\n", tmp_sf);
        _theSF = tmp_sf;
    }

    
    // Parses new test run data
    if ( sscanf(command, "%s %d %d %d %d %d %d %d",
                &command_tag,
                &tmp_numOfQueriedSF,
                &tmp_spreadThreads,
                &tmp_numOfThreads,
                &tmp_numOfTrxs,
                &tmp_selectedTrxID,
                &tmp_iterations,
                &tmp_binding) < 2 ) 
    {
        usage_cmd_TEST();
        return (SHELL_NEXT_CONTINUE);
    }


    // REQUIRED Parameters

    // 1- number of queried Scaling factor - numOfQueriedSF
    if ((tmp_numOfQueriedSF>0) && (tmp_numOfQueriedSF<=_theSF))
        numOfQueriedSF = tmp_numOfQueriedSF;
    else
        numOfQueriedSF = _theSF;
    assert (numOfQueriedSF <= _theSF);


    // OPTIONAL Parameters

    // 2- spread trxs
    spreadThreads = tmp_spreadThreads;

    // 3- number of threads - numOfThreads
    if ((tmp_numOfThreads>0) && (tmp_numOfThreads<=MAX_NUM_OF_THR)) {
        numOfThreads = tmp_numOfThreads;
        //if (spreadThreads && (numOfThreads > numOfQueriedSF))
        //numOfThreads = numOfQueriedSF;
        if (spreadThreads && ((numOfThreads % numOfQueriedSF)!=0)) {
            TRACE( TRACE_ALWAYS, "\n!!! Warning QueriedSF=(%d) and Threads=(%d) - not spread uniformly!!!\n",
                   numOfQueriedSF, numOfThreads);
        }
    }
    else {
        numOfThreads = numOfQueriedSF;
    }
    
    // 4- number of trxs - numOfTrxs
    if (tmp_numOfTrxs>0)
        numOfTrxs = tmp_numOfTrxs;

    // 5- selected trx
    mapSupTrxsConstIt stip = _sup_trxs.find(tmp_selectedTrxID);
    if (stip!=_sup_trxs.end()) {
        selectedTrxID = tmp_selectedTrxID;
    }
    else {
        TRACE( TRACE_ALWAYS, "Unsupported TRX\n");
        return (SHELL_NEXT_CONTINUE);
    }

    // 6- number of iterations - iterations
    if (tmp_iterations>0)
        iterations = tmp_iterations;

    // 8- binding type   
    mapBindPolsIt cit = _sup_bps.find(tmp_binding);
    if (cit!= _sup_bps.end()) {
        binding = tmp_binding;
    }
    else {
        TRACE( TRACE_ALWAYS, "Unsupported Binding\n");
        return (SHELL_NEXT_CONTINUE);
    }


    // call the virtual function that implements the test    
    return (_cmd_TEST_impl(numOfQueriedSF, spreadThreads, numOfThreads,
                           numOfTrxs, selectedTrxID, iterations, 
                           binding));
}



/******************************************************************** 
 *
 *  @fn:    process_cmd_MEASURE
 *
 *  @brief: Parses the MEASURE cmd and calls the virtual impl function
 *
 ********************************************************************/

int shore_shell_t::process_cmd_MEASURE(const char* command, 
                                       const char* command_tag)
{
    assert (_env);
    assert (_env->is_initialized());

    // first check if env initialized and loaded
    // try to load and abort on error
    w_rc_t rcl = _env->loaddata();
    if (rcl.is_error()) {
        return (SHELL_NEXT_QUIT);
    }

    // 0. Parse Parameters
    envVar* ev = envVar::instance();
    int numOfQueriedSF      = ev->getVarInt("measure-num-queried",DF_NUM_OF_QUERIED_SF);
    int tmp_numOfQueriedSF  = numOfQueriedSF;
    int spreadThreads        = ev->getVarInt("measure-spread",DF_SPREAD_THREADS);
    int tmp_spreadThreads    = spreadThreads;
    int numOfThreads         = ev->getVarInt("measure-num-threads",DF_NUM_OF_THR);
    int tmp_numOfThreads     = numOfThreads;
    int duration             = ev->getVarInt("measure-duration",DF_DURATION);
    int tmp_duration         = duration;
    int selectedTrxID        = ev->getVarInt("measure-trx-id",DF_TRX_ID);
    int tmp_selectedTrxID    = selectedTrxID;
    int iterations           = ev->getVarInt("measure-iterations",DF_NUM_OF_ITERS);
    int tmp_iterations       = iterations;
    eBindingType binding     = DF_BINDING_TYPE;//ev->getVarInt("measure-cl-binding",DF_BINDING_TYPE);
    eBindingType tmp_binding = binding;

    // update the SF
    int tmp_sf = ev->getSysVarInt("sf");
    if (tmp_sf) {
        TRACE( TRACE_STATISTICS, "Updated SF (%d)\n", tmp_sf);
        _theSF = tmp_sf;
    }

    
    // Parses new test run data
    if ( sscanf(command, "%s %d %d %d %d %d %d %d",
                &command_tag,
                &tmp_numOfQueriedSF,
                &tmp_spreadThreads,
                &tmp_numOfThreads,
                &tmp_duration,
                &tmp_selectedTrxID,
                &tmp_iterations,
                &tmp_binding) < 2 ) 
    {
        usage_cmd_MEASURE();
        return (SHELL_NEXT_CONTINUE);
    }


    // REQUIRED Parameters

    // 1- number of queried warehouses - numOfQueriedSF
    if ((tmp_numOfQueriedSF>0) && (tmp_numOfQueriedSF<=_theSF))
        numOfQueriedSF = tmp_numOfQueriedSF;
    else
        numOfQueriedSF = _theSF;
    assert (numOfQueriedSF <= _theSF);


    // OPTIONAL Parameters

    // 2- spread trxs
    spreadThreads = tmp_spreadThreads;

    // 3- number of threads - numOfThreads
    if ((tmp_numOfThreads>0) && (tmp_numOfThreads<=MAX_NUM_OF_THR)) {
        numOfThreads = tmp_numOfThreads;
        //if (spreadThreads && (numOfThreads > numOfQueriedSF))
        //numOfThreads = numOfQueriedSF;
        if (spreadThreads && ((numOfThreads % numOfQueriedSF)!=0)) {
            TRACE( TRACE_ALWAYS, "\n!!! Warning QueriedSF=(%d) and Threads=(%d) - not spread uniformly!!!\n",
                   numOfQueriedSF, numOfThreads);
        }
    }
    else {
        numOfThreads = numOfQueriedSF;
    }
    
    // 4- duration of measurement - duration
    if (tmp_duration>0)
        duration = tmp_duration;

    // 5- selected trx
    mapSupTrxsConstIt stip = _sup_trxs.find(tmp_selectedTrxID);
    if (stip!=_sup_trxs.end()) {
        selectedTrxID = tmp_selectedTrxID;
    }
    else {
        TRACE( TRACE_ALWAYS, "Unsupported TRX\n");
        return (SHELL_NEXT_CONTINUE);
    }

    // 6- number of iterations - iterations
    if (tmp_iterations>0)
        iterations = tmp_iterations;

    // 8- binding type   
    mapBindPolsIt cit = _sup_bps.find(tmp_binding);
    if (cit!= _sup_bps.end()) {
        binding = tmp_binding;
    }
    else {
        TRACE( TRACE_ALWAYS, "Unsupported Binding\n");
        return (SHELL_NEXT_CONTINUE);
    }

    // call the virtual function that implements the measurement    
    return (_cmd_MEASURE_impl(numOfQueriedSF, spreadThreads, numOfThreads,
                              duration, selectedTrxID, iterations,
                              binding));
}


/******************************************************************** 
 *
 *  @fn:    SIGINT_handler
 *
 *  @brief: Aborts test/measurement
 *
 ********************************************************************/

int shore_shell_t::SIGINT_handler() 
{
    if(_processing_command && !_g_canceled) {
	_g_canceled = true;
	base_client_t::abort_test();
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

int shore_shell_t::_cmd_WARMUP_impl(const int iQueriedSF, 
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

int shore_shell_t::_cmd_LOAD_impl()
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



//// EOF: shore_shell_t functions ////



/*********************************************************************
 *
 *  fake_io_delay_cmd_t: "iodelay" command
 *
 *********************************************************************/

void fake_iodelay_cmd_t::usage(void)
{
    TRACE( TRACE_ALWAYS, "IODELAY Usage:\n\n"                               \
           "*** iodelay <DELAY>\n"                      \
           "\nParameters:\n"                                            \
           "<DELAY> - the enforced fake io delay, if 0 disables fake io delay\n\n");
}

const int fake_iodelay_cmd_t::handle(const char* cmd)
{
    char cmd_tag[SERVER_COMMAND_BUFFER_SIZE];
    char iodelay_tag[SERVER_COMMAND_BUFFER_SIZE];    
    if ( sscanf(cmd, "%s %s", &cmd_tag, &iodelay_tag) < 2) {
        // prints all the env
        usage();
        return (SHELL_NEXT_CONTINUE);
    }
    assert (_env);
    int delay = atoi(iodelay_tag);
    if (!delay>0) {
        _env->disable_fake_disk_latency();
    }
    else {
        _env->enable_fake_disk_latency(delay);
    }
    return (SHELL_NEXT_CONTINUE);
}

