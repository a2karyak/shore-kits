// -*- mode:C++; c-basic-offset:4 -*-

/** @file    : test_q6.cpp
 *  @brief   : Unittest for Q6
 *  @version : 0.1
 *  @history :
 6/8/2006 : Updated to work with the new class definitions
 5/25/2006: Initial version
*/ 

#include "engine/thread.h"
#include "engine/core/stage_container.h"
#include "engine/stages/tscan.h"
#include "engine/stages/aggregate.h"
#include "engine/stages/func_call.h"
#include "engine/dispatcher.h"
#include "engine/dispatcher/dispatcher_policy_os.h"
#include "engine/util/stopwatch.h"
#include "trace.h"
#include "qpipe_panic.h"
#include "tests/common.h"
#include "workload/tpch/tpch_db.h"


#include "db_cxx.h"

using namespace qpipe;

extern uint32_t trace_current_setting;



/** @fn    : main
 *  @brief : TPC-H Q6
 */
int main(int argc, char* argv[]) {

    thread_init();
    dispatcher_policy_t* dp = new dispatcher_policy_os_t();
    trace_current_setting = TRACE_ALWAYS;


    // parse command line args
    if ( argc < 3 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <num clients> <iterations per client>\n", argv[0]);
	exit(-1);
    }
    int num_clients = atoi(argv[1]);
    if ( num_clients == 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid num clients %s\n", argv[1]);
	exit(-1);
    }
    int iterations_per_client = atoi(argv[2]);
    if ( iterations_per_client == 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid iterations per client %s\n", argv[2]);
	exit(-1);
    }



    if ( !db_open() ) {
        TRACE(TRACE_ALWAYS, "db_open() failed\n");
        QPIPE_PANIC();
    }        


    register_stage<tscan_stage_t>(1);
    register_stage<aggregate_stage_t>(num_clients);
   

    stopwatch_t timer;
    pthread_t clients[num_clients];
    for (int i = 0; i < num_clients; i++) {

        // allocate client state
        struct q6_client_info_s* cinfo = new q6_client_info_s(i, iterations_per_client, dp);
        
        // create client thread
        thread_t* client = new tester_thread_t(q6_client_main, cinfo, "CLIENT-%d", i);
        int create_ret = thread_create( &clients[i], client );
        assert(create_ret == 0);
    }
    for (int i = 0; i < num_clients; i++) {
        int join_ret = pthread_join( clients[i], NULL );
        assert(join_ret == 0);
    }    

    
    TRACE(TRACE_ALWAYS, "Query executed in %.3lf s\n", timer.time());
    if ( !db_close() )
        TRACE(TRACE_ALWAYS, "db_close() failed\n");
    return 0;
}
