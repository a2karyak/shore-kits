/* -*- mode:C++; c-basic-offset:4 -*- */
// CC -m64 -xarch=ultraT1 -xs -g -I $SHORE_INCLUDE_DIR shore_tpcc_load.cpp -o shore_tpcc_load -L $SHORE_LIB_DIR -mt -lsm -lsthread -lfc -lcommon -lpthread

#include "tests/common.h"
#include "stages/tpcc/shore/shore_tpcc_env.h"
#include "sm/shore/shore_helper_loader.h"


using namespace shore;
using namespace tpcc;


//////////////////////////////


//////////////////////////////


const char* translate_trx_id(const int trx_id) 
{
    switch (trx_id) {
    case (XCT_NEW_ORDER):
        return ("NewOrder");
        break;
    case (XCT_PAYMENT):
        return ("Payment");
        break;
    case (XCT_ORDER_STATUS):
        return ("OrderStatus");
        break;
    case (XCT_DELIVERY):
        return ("Delivery");
        break;
    case (XCT_STOCK_LEVEL):
        return ("StockLevel");
    default:
        return ("Mix");
    }
}

void print_usage(char* argv[]) 
{
    TRACE( TRACE_ALWAYS, "\nUsage:\n" \
           "test <NUM_QUERIED> [<SPREAD> <NUM_THRS> <NUM_TRXS> <TRX_ID> <ITERATIONS>]\n" \
           "\nParameters:\n" \
           "<NUM_QUERIED> : The number of WHs queried (queried factor)\n" \
           "<SPREAD>      : Whether to spread threads to WHs (0=No, Otherwise=Yes, Default=No) (optional)\n" \
           "<NUM_THRS>    : Number of threads used (optional)\n" \
           "<NUM_TRXS>    : Number of transactions per thread (optional)\n" \
           "<TRX_ID>      : Transaction ID to be executed (0=mix) (optional)\n" \
           "<ITERATIONS>  : Number of iterations (Default=5) (optional)\n");
}


int main(int argc, char* argv[]) 
{
    // initialize cordoba threads
    thread_init();


    TRACE_SET( TRACE_ALWAYS | TRACE_STATISTICS | TRACE_NETWORK | TRACE_CPU_BINDING
               //              | TRACE_QUERY_RESULTS
               //              | TRACE_PACKET_FLOW
               //               | TRACE_RECORD_FLOW
               //               | TRACE_TRX_FLOW
               //               | TRACE_DEBUG
              );


    /* 0. Parse Parameters */
    int numOfQueriedWHs = DF_NUM_OF_QUERIED_WHS;
    int spreadThreads   = DF_SPREAD_THREADS_TO_WHS;
    int numOfThreads    = DF_NUM_OF_THR;
    int numOfTrxs       = DF_TRX_PER_THR;
    int selectedTrxID   = DF_TRX_ID;
    int iterations      = DF_NUM_OF_ITERS;

    if (argc<2) {
        print_usage(argv);
        return (1);
    }

    int tmp = atoi(argv[1]);
    if (tmp>0)
        numOfQueriedWHs = tmp;

    if (argc>2)
        spreadThreads = atoi(argv[2]);
    
    if (argc>3) {
        tmp = atoi(argv[3]);
        if ((tmp>0) && (tmp<=MAX_NUM_OF_THR)) {
            numOfThreads = tmp;
            if (spreadThreads && (tmp > numOfQueriedWHs))
                numOfThreads = numOfQueriedWHs;
        }
    }

    if (argc>4) {
        tmp = atoi(argv[4]);
        if (tmp>0)
            numOfTrxs = tmp;
    }

    if (argc>5) {
        selectedTrxID = atoi(argv[5]);
    }

    if (argc>6) {
        iterations = atoi(argv[6]);
    }

    // Print out configuration
    TRACE( TRACE_ALWAYS, "\n\n" \
           "Queried WHs    : %d\n" \
           "Spread Threads : %s\n" \
           "Num of Threads : %d\n" \
           "Num of Trxs    : %d\n" \
           "Trx            : %s\n" \
           "Iterations     : %d\n", 
           numOfQueriedWHs, (spreadThreads ? "Yes" : "No"), 
           numOfThreads, numOfTrxs, translate_trx_id(selectedTrxID), 
           iterations);

    /* 1. Instanciate the Shore Environment */
    _g_shore_env = new ShoreTPCCEnv("shore.conf");

    // the initialization must be executed in a shore context
    db_init_smt_t* initializer = new db_init_smt_t(c_str("init"), _g_shore_env);
    initializer->fork();
    initializer->join();        
    if (initializer) {
        delete (initializer);
        initializer = NULL;
    }    
    _g_shore_env->print_sf();
    
    test_smt_t* testers[MAX_NUM_OF_THR];
    for (int j=0; j<iterations; j++) {

        TRACE( TRACE_ALWAYS, "Iteration [%d of %d]\n",
               (j+1), iterations);
        TRACE( TRACE_ALWAYS, "Starting (%d) threads with (%d) trxs each...\n",
               numOfThreads, numOfTrxs);

	stopwatch_t timer;
        int wh_id = 0;
        for (int i=0; i<numOfThreads; i++) {
            // create & fork (numOfThreads) testing threads
            if (spreadThreads)
                wh_id = i+1;
            testers[i] = new test_smt_t(_g_shore_env, MT_NUM_OF_TRXS,
                                        wh_id, selectedTrxID,
                                        numOfTrxs, 0, c_str("tt%d", i));
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
               "Threads: (%d)\nTrxs:    (%d)\nSecs:    (%.2f)\nTPS:     (%.2f)\n",
               numOfThreads, numOfTrxs, delay, numOfThreads*numOfTrxs/delay);
    }

    // close Shore env
    close_smt_t* clt = new close_smt_t(_g_shore_env, c_str("clt"));
    clt->fork();
    clt->join();
    if (clt->_rv) {
        TRACE( TRACE_ALWAYS, "Error in closing thread...\n");
        return (1);
    }

    if (clt) {
        delete (clt);
        clt = NULL;
    }

    return (0);
}
