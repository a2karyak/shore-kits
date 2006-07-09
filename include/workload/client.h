/** @file    : client.h
 *  @brief   : Definition of the client class
 *  @version : 0.1
 *  @history :
 6/11/2006: Initial version
*/ 

/* Definition of QPipe server workload related classes:
   client_counter : Singleton class for unique client numbers.
   client_t       : Corresponds to a qpipe client. Such a client is declared
                    by the selected Query or SQL text, the number of queries to
                    completed and the think time.
*/

#ifndef __WL_CLIENT_H
#define __WL_CLIENT_H


/* Standard */
#include <cstdlib>
#include <string>

/* QPipe Engine */
#include "engine/thread.h"
#include "engine/util/shuffle.h"
#include "trace.h"

/* QPipe WL */
#include "workload/workload.h"
#include "workload/job.h"
#include "workload/job_directory.h"

// include me last!!!
#include "engine/namespace.h"


#define EMPTY_STRING ""


using std::string;

class workload_factory;
class workload_t;


/* Wrapper functions for the creation of threads */

/* starts a specific client */
void* start_cl(void* arg);
    

/**
 * class client_counter
 *
 * @brief : Singleton class that returns a unique identifier for all the
 * clients in the system
 */

class client_counter {

 private:
    /* counter and mutex */
    int client_cnt;
    pthread_mutex_t cnt_mutex;

    /* instance and mutex */
    static client_counter* clcInstance;
    static pthread_mutex_t instance_mutex;

    client_counter();
    ~client_counter();

 public:
    static client_counter* instance();
    void get_next_client_id(int* packet_id);
};



/**
 * class client_t
 *
 * @brief : Class that represents a QPipe client
 */

class client_t : public thread_t {
private:

    // client workload
    int clUniqueID;
    int clSelQuery;
    int clThinkTime;
    int clNumOfQueries;

    // SQL text
    string clSQL;

    // pointer to workload
    workload_t* myWorkload;

    // initialization function for each client instance
    void init(const char* format, va_list ap);
  
public:

    // client statistics
    int clCompleted;

    // time variables
    time_t clStartTime;
    time_t clEndTime;

  
    client_t(const char* format, ...);

    /* TODO: Add SQL support */
    client_t(int query, int think, int iter, const char* format, ...); 
    client_t(const client_t& rhs, const char* format, ...);

    ~client_t();

    /* execute */
    void* run();
    
    /* returns client unique ID */
    int get_unique_id() { return (clUniqueID); }

    // access workload variable
    void set_workload(workload_t* aWorkload);
    workload_t* get_workload() { return (myWorkload); }

    // simple client description
    void desc();

    // members access methods
    int get_sel_query() { return (clSelQuery); }
    int get_num_iter() { return (clNumOfQueries); }
    int get_think_time() { return (clThinkTime); }
    void set_sql( const string aSQL );
    string get_sel_sql() { return (clSQL); }
};


#include "engine/namespace.h"


#endif	// __WL_CLIENT_H
