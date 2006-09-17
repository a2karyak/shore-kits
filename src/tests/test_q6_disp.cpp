/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file    : test_q6.cpp
 *  @brief   : Unittest for Q6
 *  @version : 0.1
 *  @history :
 6/8/2006 : Updated to work with the new class definitions
 5/25/2006: Initial version
*/ 

#include "stages.h"
#include "scheduler.h"
#include "tests/common.h"
#include "workload/common.h"

#include "workload/tpch/tpch_db.h"

using namespace qpipe;



/** @fn    : main
 *  @brief : TPC-H Q6
 */
int main(int argc, char* argv[]) {

    thread_init();
    TRACE_SET(TRACE_ALWAYS);


    // parse command line args
    if ( argc < 4 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <num clients> <iterations per client> OS|RR_CPU|QUERY_CPU [cachesize]\n", argv[0]);
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
    scheduler::policy_t* dp = NULL;
    char* dpolicy = argv[3];
    if ( !strcmp(dpolicy, "OS") )
        dp = new scheduler::policy_os_t();
    if ( !strcmp(dpolicy, "RR_CPU") )
        dp = new scheduler::policy_rr_cpu_t();
    if ( !strcmp(dpolicy, "QUERY_CPU") )
        dp = new scheduler::policy_query_cpu_t();
    if ( dp == NULL ) {
	TRACE(TRACE_ALWAYS, "Unrecognized dispatcher policy: %s\n", argv[3]);
	exit(-1);
    }

    u_int32_t flags = DB_RDONLY|DB_THREAD;
    u_int32_t db_cache_size_gb=BDB_BUFFER_POOL_SIZE_GB;
    u_int32_t db_cache_size_bytes=BDB_BUFFER_POOL_SIZE_BYTES;
    if ( argc > 4 ) {
        db_cache_size_bytes = atoi(argv[4]);
        if ( db_cache_size_bytes == 0 ) {
            TRACE(TRACE_ALWAYS, "Invalid cache size bytes %s\n", argv[4]);
            exit(-1);
        }
    }
        
        
    db_open(flags, db_cache_size_gb, db_cache_size_bytes);


    register_stage<tscan_stage_t>(num_clients);
    register_stage<aggregate_stage_t>(num_clients);
   

    stopwatch_t timer;
    pthread_t clients[num_clients];
    for (int i = 0; i < num_clients; i++) {

        // allocate client state
        struct q6_client_info_s* cinfo = new q6_client_info_s(i, iterations_per_client, dp);
        
        // create client thread
        thread_t* client = new tester_thread_t(q6_client_main, cinfo, c_str("CLIENT-%d", i));
        clients[i] = thread_create(client );
    }
    for (int i = 0; i < num_clients; i++) 
        thread_join<void>(clients[i]);

    
    TRACE(TRACE_ALWAYS, "Query executed in %.3lf s\n", timer.time());
    db_close();
    return 0;
}
