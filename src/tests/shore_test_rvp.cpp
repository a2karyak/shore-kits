/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_rvp.cpp
 *
 *  @brief:  Tests the performance of the Rendez-Vouz points.
 *
 *  @author: Ippokratis Pandis (ipandis)
 */


#include "tests/common.h"
#include "stages/tpcc/shore/shore_tpcc_env.h"

#include "util/shell.h"
#include "util/countdown.h"


using namespace shore;
using namespace tpcc;



//// Default values of the parameters for the run

// default number of checkpoints
const int DF_NUM_OF_CHK = 5;



///////////////////////////////////////////////////////////
// @class test_rvp_smt_t
//
// @brief An smthread-based class for testing the RVP

class test_rvp_smt_t : public thread_t 
{
private:

    ShoreTPCCEnv* _env;    
    
    countdownPtr*  _pp_rvp;
    int _nochks;

public:
    int	_rv;
    
    test_rvp_smt_t(ShoreTPCCEnv* env, 
                   countdownPtr* pprvp, int numOfChks,
                   c_str tname) 
	: thread_t(tname), _nochks(numOfChks), _pp_rvp(pprvp),
          _rv(0)
    {
        assert (_nochks>0);
        assert (_pp_rvp);
    }

    ~test_rvp_smt_t() { }

    w_rc_t test_rvps();


    // thread entrance
    void work() {

        // run test
        w_rc_t e = test_rvps();
        if (e.is_error()) {
            TRACE( TRACE_ALWAYS, "RVP testing failed [0x%x]\n", e.err_num());
            assert (false); // TODO - error occured
        }

        _rv = e.is_error();
    }

    /** @note Those two functions should be implemented by every
     *        smthread-inherited class that runs using run_smthread()
     */
    inline int retval() { return (_rv); }

}; // EOF: test_rvp_smt_t


w_rc_t test_rvp_smt_t::test_rvps()
{
    int waited=0;
    for (int i=0; i<_nochks; i++) {

        if (_pp_rvp[i]->post()) {
            // last caller
            TRACE( TRACE_DEBUG, "finishing (%d)  - waited (%d)\n", i, waited);
        }
        else {
            // wait (spinning)
            while (_pp_rvp[i]->remaining()>0)
                waited++;

            TRACE( TRACE_DEBUG, "going next (%d) - waited (%d)\n", i, waited);
        }
    }    

    return (RCOK);
}


/** EOF: test_rvp_smt_t functions */


//////////////////////////////



//////////////////////////////


class rvp_test_shell_t : public shell_t 
{ 
public:

    rvp_test_shell_t(const char* prompt) 
        : shell_t(prompt)
    {
    }

    ~rvp_test_shell_t() 
    { 
        TRACE( TRACE_ALWAYS, "Exiting... (%d) commands processed\n",
               get_command_cnt());
    }

    const int register_commands() { return (SHELL_NEXT_CONTINUE); }

    // shell interface
    int process_command(const char* cmd, const char* cmd_tag);
    int print_usage(const char* command);

}; // EOF: rvp_test_shell_t


/** rvp_test_shell_t interface */


int rvp_test_shell_t::print_usage(const char* command) 
{
    assert (command);

    TRACE( TRACE_ALWAYS, "\nUsage:\n" \
           "rvp <NUM_THREADS> [<NUM_CHKS> <ITERATIONS>]\n" \
           "\nParameters:\n" \
           "<NUM_THREADS> : Number of concurrent threads (Default=5)\n" \
           "<NUM_CHKS>    : Number of trxs per thread (Default=5) (optional)\n" \
           "<ITERATIONS>  : Number of iterations (Default=5) (optional)\n");
    
    return (SHELL_NEXT_CONTINUE);
}


int rvp_test_shell_t::process_command(const char* cmd, const char* cmd_tag)
{
    /* 0. Parse Parameters */
    int numOfThreads        = DF_NUM_OF_THR;
    int tmp_numOfThreads    = DF_NUM_OF_THR;
    int numOfChks           = DF_NUM_OF_CHK;
    int tmp_numOfChks       = DF_NUM_OF_CHK;
    int iterations          = DF_NUM_OF_ITERS;
    int tmp_iterations      = DF_NUM_OF_ITERS;

    // Parses new test run data
    if ( sscanf(cmd, "%s %d %d %d",
                &cmd_tag,
                &tmp_numOfThreads,
                &tmp_numOfChks,
                &tmp_iterations) < 2)
    {
        print_usage(cmd_tag);
        return (SHELL_NEXT_CONTINUE);
    }


    // REQUIRED Parameters

    // 1- number of threads - numOfThreads
    if ((tmp_numOfThreads>0) && (tmp_numOfThreads<=MAX_NUM_OF_THR)) {
            numOfThreads = tmp_numOfThreads;
    }


    // OPTIONAL Parameters
    
    // 2- number of checks - numOfChks
    if (tmp_numOfChks>0)
        numOfChks = tmp_numOfChks;

    // 3- number of iterations - iterations
    if (tmp_iterations>0)
        iterations = tmp_iterations;


    // Print out configuration
    TRACE( TRACE_ALWAYS, "\n\n" \
           "Num of Threads  : %d\n" \
           "Num of Chks     : %d\n" \
           "Iterations      : %d\n", 
           numOfThreads, numOfChks, iterations);

    test_rvp_smt_t* testers[MAX_NUM_OF_THR];
    countdownPtr* pprvps;

    for (int j=0; j<iterations; j++) {

        TRACE( TRACE_ALWAYS, "Iteration [%d of %d]\n",
               (j+1), iterations);

        // create the individual checkpoints
        // and set them in the array
        pprvps = new countdownPtr[numOfChks];
        for (int k=0; k<numOfChks; k++) {
            pprvps[k] = new countdown_t(numOfThreads);
        }

	stopwatch_t timer;

        for (int i=0; i<numOfThreads; i++) {

            // create & fork testing threads
            testers[i] = new test_rvp_smt_t(NULL, pprvps, numOfChks,
                                            c_str("tt%d", i));
            testers[i]->fork();

        }

        /* 2. join the tester threads */
        for (int i=0; i<numOfThreads; i++) {

            testers[i]->join();
            if (testers[i]->_rv) {
                TRACE( TRACE_ALWAYS, "Error in testing...\n");
                TRACE( TRACE_ALWAYS, "Exiting...\n");
                assert (false);
            }    
            delete (testers[i]);
        }

	double delay = timer.time();
        TRACE( TRACE_ALWAYS, "*******\n" \
               "Threads: (%d)\n" \
               "Chks:    (%d)\n" \
               "Secs:    (%.2f)\n" \
               "ChksPS:  (%.2f)\n",
               numOfThreads, numOfChks, delay, 
               numOfChks/delay);

        for (int k=0; k<numOfChks; k++) {
            if (pprvps[k])
                delete (pprvps[k]);        
        }
        if (pprvps)
            delete [] pprvps;

    }

    return (SHELL_NEXT_CONTINUE);
}

/** EOF: rvp_test_shell_t functions */




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
               //               | TRACE_TRX_FLOW
                              | TRACE_DEBUG
              );

    /* 1. Start processing commands */
    rvp_test_shell_t tshell("(rvp) ");
    tshell.start();

    return (0);
}


