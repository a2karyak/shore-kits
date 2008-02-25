/* -*- mode:C++; c-basic-offset:4 -*- */

#include "core.h"
#include "workload/workload.h"
#include "workload/client_sync.h"
#include "workload/workload_client.h"


ENTER_NAMESPACE(workload);


/*********************************************************************
 *
 *  @brief Initialize the corresponding threads of clients, start
 *  them, and then join on them. This method will return only when all
 *  the clients have completed their runs.
 *
 *********************************************************************/

bool workload_t::run(results_t &results) {
   
    
    // store workload parameters
    results.num_clients    = _num_clients;
    results.think_time     = _think_time;
    results.num_iterations = _num_iterations;
    results.client_times   = new execution_time_t[_num_clients];


    // create client threads for this workload
    int clients_created = 0;
    client_sync_t client_sync;

#ifdef USE_SMTHREAD_AS_BASE
    typedef sthread_t* psthread;
    array_guard_t<psthread> clients = new psthread[_num_clients];
#else
    array_guard_t<pthread_t> client_ids = new pthread_t[_num_clients];
#endif

    for (int client_index = 0; client_index < _num_clients; client_index++) {

        // create another workload client
        try {
            // create new client thread
            c_str client_name("%s.%d", _name.data(), client_index);
            workload_client_t* client =
                new workload_client_t(client_name,
                                      &results.client_times[client_index],
                                      _driver,
                                      _driver_arg,
                                      &client_sync,
                                      _num_iterations,
                                      _think_time);

#ifdef USE_SMTHREAD_AS_BASE
            client->fork();
            clients[client_index] = client;
#else
            client_ids[client_index] = thread_create(client);
#endif
        }
        catch (QPipeException &e) {

            // Regardless of what errors are raised, we should destroy
            // all the threads we have created so far before
            // propagating the error.
            client_sync.signal_error();
            
            // wait for client threads to receive error message
#ifdef USE_SMTHREAD_AS_BASE
            wait_for_sthread_clients(clients, clients_created);
#else
            wait_for_clients(client_ids, clients_created);
#endif
            
            // now that we have collected clients, propagate exception
            // up the call stack
            throw e;
        }

        clients_created++;
    }

    // record start time
    stopwatch_t timer;
   
    // run workload and wait for clients to finish
    client_sync.signal_continue();

    // wait for the child threads to finish
#ifdef USE_SMTHREAD_AS_BASE
    wait_for_sthread_clients(clients, clients_created);
#else
    wait_for_clients(client_ids, clients_created);
#endif    

    // record finish time
    results.total_time = timer.time();

    TRACE(TRACE_DEBUG, "Open Fifo count: %d\n", qpipe::tuple_fifo::open_fifos());
    return true;
}



/*********************************************************************
 *  
 *  @fn:    wait_for_clients
 *
 *  @brief: Wait for created clients to exit. All clients are created
 *  in the joinable state, so we just need to wait for them to
 *  exit. This method should only be called AFTER invoking
 *  signal_continue() or signal_error() on the client_wait_t instance
 *  we passed the client threads. Otherwise, we will have
 *  deadlock. The clients will be waiting to be signaled and the
 *  runner will be waiting for them to exit.
 *
 *  @param: thread_ids - An array of thread IDs for the created clients.
 *
 *  @param: num_thread_ids - The number of valiid thread IDs in the
 *  thread_ids array.
 *
 *********************************************************************/

void workload_t::wait_for_clients(pthread_t* thread_ids, 
                                  int num_thread_ids) 
{
    // wait for client threads to receive error message
    for (int i = 0; i < num_thread_ids; i++) {
        // join should not really fail unless we are doing
        // something seriously wrong...
        int join_ret = pthread_join(thread_ids[i], NULL);
        assert(join_ret == 0);
    }
}


EXIT_NAMESPACE(workload);
