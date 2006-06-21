// -*- mode:C++; c-basic-offset:4 -*-

/** @file    : qpipe_wl.h
 *  @brief   : Header for workload related classes
 *  @version : 0.1
 *  @history :
 6/11/2006: Initial version
*/ 

/* Definition of QPipe server workload related classes:
   workload_factory : Singleton class that creates corresponding workload.
   workload_t       : Contains a list of clients and bookeeping variables.
   client_counter   : Singleton class for unique client numbers.
   client_t         : Corresponds to a qpipe client. Such a client is declared
   by the selected Query or SQL text, the number of queries to
   completed and the think time.
*/

# ifndef __QPIPE_WL_H
# define __QPIPE_WL_H


/* Standard */
#include <cstdlib>
#include <string>

/* QPipe Engine */
#include "engine/thread.h"
#include "engine/util/shuffle.h"
#include "trace.h"
#include "qpipe_panic.h"

#include "tests/common/tester_thread.h"

/* QPipe Workload */
#include "parser.h"


// include me last!!!
#include "engine/namespace.h"


using std::string;

class client_t;
class client_counter;
class workload_t;
class workload_factory;
class parser_t;

extern pthread_mutex_t stats_mutex;

/* No more than 20 active clients on the same time */
#define MAX_CLIENTS 20

/* No more than 20 active workloads on the same time */
#define MAX_WORKLOADS 20 

#define EMPTY_STRING ""


/* Wrapper functions for the creation of threads */

/* start a specific workload */
void* start_wl(void* arg);

/* starts a specific client */
void* start_cl(void* arg);
    

/**
 * class workload_factory
 *
 * @brief: Singleton class that contains a list of workloads as well as a parser
 */

class workload_factory {

 private:

    /* status */
    int isInteractive;
    
    /* parser */
    parser_t* theParser;

    /* mutex protecting the workload creation */
    pthread_mutex_t theWLCreation_mutex;

    /* handles and workloads */
    int theWLCounter;
    workload_t** theWLs;
    pthread_t* theWLsHandles;
    int* theWLsStatus;

    /* instance and mutex */
    static workload_factory* workloadFactoryInstance;
    static pthread_mutex_t instance_mutex;

    workload_factory();
    ~workload_factory();

 public:

    /* counter of totally completed queries and corresponding mutex */
    int theCOQueries;
    pthread_mutex_t theStat_mutex;

    /* instance call */
    static workload_factory* instance();

    /* add a new workload consisting of only 1 client */
    int attach_client(client_t new_client);

    /* add a new workload consisting of N same clients */
    int attach_clients(int clients, client_t templ);  

    /* create a workload and return its index */
    int create_wl(const int noClients, const int thinkTime,
                  const int noCompleted, const int selQuery,
                  const string selSQL = NULL);

    /* print info */
    void print_info();

    /* print info for each started workload */
    void print_wls_info();

    /* updates the status array entry for the specific workload */
    void update_wl_status(int iWLIdx, int iStatus);

    /* returns a reference to the parser instance */
    parser_t* get_parser( ) { return (theParser); }
};



/**
 * class workload_t
 *
 * @brief: Class that corresponds to a QPipe workload. Consists of a list of clients and
 * corresponding functions that initialize and start them.
 */

class workload_t : public thread_t {

 private:

    /* client workload */
    int wlNumClients;
    int wlSelQuery;
    string wlSQLText;
    int wlThinkTime;
    int wlNumQueries;

    /* structures that store the running clients */
    pthread_t* clientHandles;
    client_t** runningClients;

    /* index in the factory array */
    int myIdx;

    /* flag if started */
    int wlStarted;

 public:

    /* timekeeping variables */
    time_t wlStartTime;
    time_t wlEndTime;
    int queriesCompleted ;

    /* protects the various counters */
    pthread_mutex_t workload_mutex;

    /* constructor - destructor */
    workload_t();
    ~workload_t();

    /* set the workload parameters */
    int set_run(const int noClients, const int thinkTime, const int noQueries,
                const int selQuery, const string selSQL = NULL);
    
    /* creates the client threads and starts executing */
    void* run();

    /* prints out the runtime information about the workload */
    void get_info();

    /* indicates if the workload has started */
    int isStarted() { return (wlStarted); }

    /* sets the factory index */
    void set_idx(int factoryWLIdx);

    /* returns the factory index */
    int get_idx() { return (myIdx); }
};



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

    // initialization function for each instance
    void init();
  
 public:

    // client statistics
    int clCompleted;

    // time variables
    time_t clStartTime;
    time_t clEndTime;

  
    client_t();
    client_t(int query, int think, int iter, const string sql = NULL);
    client_t(const client_t& rhs);

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
    string get_sel_sql() { return (clSQL); }
};


#include "engine/namespace.h"


#endif	// __QPIPE_WL_H
