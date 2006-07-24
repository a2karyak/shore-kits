/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/workload.h"
#include "engine/core/exception.h"
#include "workload/client_sync.h"
#include "workload/client.h"
#include "engine/thread.h"



using namespace qpipe;



/**
 *  @brief Wait for created clients to exit.
 */
void workload_t::wait_for_clients(pthread_t* thread_ids, int num_thread_ids) {

    // wait for client threads to receive error message
    for (int i = 0; i < num_thread_ids; i++) {
        // join should not really fail unless we are doing
        // something seriously wrong...
        int join_ret = pthread_join(thread_ids[i], NULL);
        assert(join_ret == 0);
    }
}



/**
 *  @brief Initializes the corresponding threads of clients, starts
 *  them, and then joins them. It will return only when all the
 *  clients have their requests completed.
 */

bool workload_t::run(results_t &results) {
   
    
    // store workload parameters
    results.num_clients    = _num_clients;
    results.think_time     = _think_time;
    results.num_iterations = _num_iterations;
    execution_time_t::reset(&results.total_time);
    results.client_times   = new execution_time_t[_num_clients];


    // create client threads for this workload
    int clients_created = 0;
    pthread_t client_ids[_num_clients];
    client_sync_t client_sync;

    for (int client_index = 0; client_index < _num_clients; client_index++) {

        // create another workload client
        try {
            // create new client thread
            c_str client_name("%s.%d", _name.data(), client_index);
            workload_client_t* client =
                new workload_client_t(client_name,
                                      &results.client_times[client_index],
                                      _driver,
                                      &client_sync,
                                      _num_iterations,
                                      _think_time);
            if ( thread_create( &client_ids[client_index], client) ) {
                // raise an exception and let catch statement deal
                // with it...
                TRACE(TRACE_ALWAYS, "thread_create() failed\n");
                throw EXCEPTION(QPipeException, "thread_create() failed");
            }
        }
        catch (QPipeException &e) {

            // Regardless of what errors are raised, we should destroy
            // all the threads we have created so far before
            // propagating the error.
            client_sync.signal_error();
            
            // wait for client threads to receive error message
            wait_for_clients(client_ids, clients_created);
            
            // now that we have collected clients, propagate exception
            // up the call stack
            throw e;
        }

        clients_created++;
    }


    // record start time
    results.total_time.start();

    
    // run workload and wait for clients to finish
    client_sync.signal_continue();
    wait_for_clients(client_ids, clients_created);

    
    // record finish time
    results.total_time.stop();


    return true;
}




#if 0
/** @fn    : get_info(int)
 *  @brief : Helper function, outputs information about the workload setup
 *  @param : int show_stats - If show_stats is set then it displays throughput and other statistics
 */

void workload_t::get_info(int show_stats) {

    TRACE( TRACE_DEBUG, "RUN parameters: CLIENTS: %d\tTHINKTIME: %d\tNOCOMPLQUERIES: %d\t QUERY: %d\n", 
           wlNumClients, wlThinkTime, wlNumQueries, wlSelQuery);

    if (show_stats) {
        // print final statistics
        queriesCompleted = wlNumClients * wlNumQueries;
        
        /* queries per hour */      
        float tpmC = (60.0 * queriesCompleted) / (wlEndTime - wlStartTime);
        
        fprintf(stdout, "~~~\n");
        fprintf(stdout, "~~~ Clients           = %d \n", wlNumClients);
        fprintf(stdout, "~~~ Think Time        = %d \n", wlThinkTime);
        fprintf(stdout, "~~~ Iterations        = %d \n", wlNumQueries);
        fprintf(stdout, "~~~ Query             = %d \n", wlSelQuery);  
        fprintf(stdout, "~~~ Completed Queries = %d \n", queriesCompleted);
        fprintf(stdout, "~~~ Duration          = %ld \n", (wlEndTime - wlStartTime));
        fprintf(stdout, "~~~\n");
        fprintf(stdout, "~~~ Throughput        = %.2f queries/min\n", tpmC);
        fprintf(stdout, "~~~\n");
    }
}
#endif
