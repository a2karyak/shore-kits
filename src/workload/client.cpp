// -*- mode:C++; c-basic-offset:4 -*-

/** @file    : client.cpp
 *  @brief   : Workload client related classes
 *  @version : 0.1
 *  @history :
 6/11/2006: Initial version
*/ 

/*  Includes the implementation of the classes:
    client_counter
    client_t
*/

#include "workload/client.h"


// include me last!!!
#include "engine/namespace.h"



// MIX OF QUERIES
const int MIX[] = { 1, 6, 4 };
const int MIX_SIZE = sizeof(MIX)/sizeof(int);




//////////////////////////////////
// class client_counter

client_counter* client_counter::clcInstance = NULL;

pthread_mutex_t client_counter::instance_mutex = PTHREAD_MUTEX_INITIALIZER;


/* @fn    : Construcotr
 * @brief : Initializes the  client counter and its  mutex
 */

client_counter::client_counter() {

    if (pthread_mutex_init(&cnt_mutex, NULL)) {
        TRACE( TRACE_ALWAYS, "Error initializing client counter mutex\n");
    }

    client_cnt = 0;
}



/* @fn    : instance()
 * @brief : Returns the unique instance of the class (singleton)
 */

client_counter* client_counter::instance() {

    // first check without concern of locking
    if ( clcInstance == NULL ) {

        // instance does not exist yet
        pthread_mutex_lock(&instance_mutex);

        // we have the lock but another thread may have gotten the lock first
        if (clcInstance == NULL) {
            clcInstance = new client_counter();
        }

        // release instance mutex
        pthread_mutex_unlock(&instance_mutex);
    }

    return (clcInstance);
}


/* @fn    : Destructor
 * @brief : Destroys the mutex
 */

client_counter::~client_counter() {
  
    pthread_mutex_destroy(&cnt_mutex);
    pthread_mutex_destroy(&client_counter::instance_mutex);
}


/* @fn    : get_next_client_id(int* )
 * @brief : Sets the current value of the client counter to the passed reference
 *          and increases it
 */

void client_counter::get_next_client_id(int* cl_id) {
 
    pthread_mutex_lock(&cnt_mutex);
    *cl_id = client_cnt;
    TRACE(TRACE_DEBUG, "New QPipe client: %d\n", client_cnt);
    client_cnt = (client_cnt + 1) % (INT_MAX - 1);
    pthread_mutex_unlock(&cnt_mutex);
}





////////////////////
// class client_t


/** @fn    : init(const char*, va_list)
 *  @brief : Initialization function of a client
 */

void client_t::init(const char* format, va_list ap) {

    // receives a unique id
    client_counter * tc = client_counter::instance();
    tc->get_next_client_id(&clUniqueID);

    // set thread name
    init_thread_name_v(format, ap);

    // initializes variables
    clSelQuery = 0;
    clThinkTime = 0;
    clNumOfQueries = 1;
    clStartTime = 0;
    clEndTime = 0;
    clSQL = EMPTY_STRING;
    myWorkload = NULL;  
}


/** @fn    : Empty constructor
 *  @brief : Calls the init function
 */

client_t::client_t(const char* format, ...) : thread_t() {
    
    // standard initialization
    va_list ap;
    va_start(ap, format);
    init(format, ap);
    va_end(ap);  
}



/** @fn    : Constructor
 *  @brief : Initializes a new instance, by getting a unique client id,
 *           and setting the various variables.
 */

client_t::client_t(int query, int think, int iter, const char* format, ...) : thread_t() {
    
    // standard initialization
    va_list ap;
    va_start(ap, format);
    init(format, ap);
    va_end(ap);
    
    if (!(query<0)) {
        clSelQuery = query;
    }
  
    if (think > 0) {
        clThinkTime = think;
    }

    if (iter > 0) {
        clNumOfQueries = iter;
    }
}



/** @fn    : Copy constructor
 *  @brief : Creates a new instance by copying each field of the passed instance. 
 */

client_t::client_t(const client_t& rhs, const char* format, ...) : thread_t() {

    if (&rhs != NULL) {
        
        /* format thread name */
        va_list ap;
        va_start(ap, format);
        init(format, ap);
        va_end(ap);
        
        clUniqueID = rhs.clUniqueID;
        clSelQuery = rhs.clSelQuery;
        clThinkTime = rhs.clThinkTime;
        clNumOfQueries = rhs.clNumOfQueries;
        clStartTime = rhs.clStartTime;
        clEndTime = rhs.clEndTime;

        // copies the sql text, if any
        if (rhs.clSQL.size() > 0) {
            clSQL = rhs.clSQL;
        }

        if (rhs.myWorkload) {
            myWorkload = rhs.myWorkload;
        }
    }
}



/** @fn    : Destructor
 *  @brief : If SQL text is set, it deallocates it.
 */

client_t::~client_t() {

}



/** @fn    : set_workload(workload_t*)
 *  @brief : Sets the corresponding workload for the client
 */

void client_t::set_workload(workload_t* aWorkload) {

    if (aWorkload != NULL) {
        myWorkload = aWorkload;
    }
}



/** @fn    : set_sql(string)
 *  @brief : Sets the corresponding workload for the client
 */

void client_t::set_sql(string aSQL) {

    if (aSQL.size() > 0) {
        clSQL = aSQL;
    }
}




/** @fn     : run()
 *  @brief  : Submits the specified query for a number of iterations
 *            and with a think time interval
 */

void* client_t::run() {
    
    TRACE( TRACE_ALWAYS, "CLIENT_ID=%d Q=%d IT=%d TT=%d\n", clUniqueID, clSelQuery, clNumOfQueries, clThinkTime );

    /* return if corresponding client not properly initialized */
    if (clNumOfQueries <= 0) {
        TRACE( TRACE_ALWAYS, "Illegal client iterations = %d\n", clNumOfQueries);
        return ((void*)1);
    }

    float tpmC = 0.0;  
    int toExecute = 0;
    char* sJobCmd = (char*)malloc(sizeof(char)*3);
    job_driver_t* job_driver = NULL;
    
    // deck to shuffle
    // allocate clNumOfQueries places for the deck
    int* deck;
    int deck_size;

    int i = 0;

    deck = (int*)malloc(sizeof(int)*clNumOfQueries);
    deck_size = clNumOfQueries;

    assert(deck);

    // if specified query
    if (clSelQuery >  0) {

        // fill the deck only with the requested query
        for (i = 0; i < deck_size; i++) {
            deck[i] = clSelQuery;
        }
    }
    // otherwise fill it with the mix of selected queries and shuffle 
    else {    

        for (i =0; i < deck_size; i++) {
            deck[i] = MIX[i % MIX_SIZE];
        }
    
        // TRACE_DEBUG("Client %d. Before Shuffle: ", client.clUniqueID); print_array( deck, MIX_SIZE);
        init_srand((uint)pthread_self());
        do_shuffle( deck, deck_size );
    }

    printf("Client %d executing queries: ", clUniqueID);
    print_array( deck, deck_size );

    // get a reference to the workload factory
    workload_factory* wf = workload_factory::instance();

    // record the number of totally completed queries at this point of time
    int complWhenStarted = wf->theCOQueries;

    // start of client response time
    clStartTime = time(&clStartTime);

    /* iterate tpchNumOfQueries times */
    for (i = 0; i < deck_size; ++i) {
        
        if (clSQL.size() > 0) {
            TRACE( TRACE_DEBUG, "SHOULD EXECUTE GENERIC QUERY\nSQL=\t%s\n", clSQL.c_str());
            // call the generic query with cliet.clSQL as parameter
            continue;
        }

        
        toExecute = deck[i];
        fprintf(stdout, "Client: %d Exec: %d\t Completed: [%d/%d]\n", clUniqueID, toExecute, i, deck_size);
        
        snprintf(sJobCmd, 3, "%d", toExecute);        
        
        job_driver = job_directory::get_job_driver(sJobCmd);
        if (job_driver == NULL) {

            TRACE( TRACE_ALWAYS, "No Job Driver found\n");
            continue;
        }
        
        // job driver found. execute compiled query
        job_driver->drive();
        
        time_t cur_time = time(&cur_time);

        // increase the total number of completed queries
        pthread_mutex_lock(&wf->theStat_mutex);
        wf->theCOQueries++;
        pthread_mutex_unlock(&wf->theStat_mutex);

        // increase the number of completed queries for the workload
        pthread_mutex_lock(&get_workload()->workload_mutex);
        get_workload()->queriesCompleted++;
        pthread_mutex_unlock(&get_workload()->workload_mutex);

        /* queries per hour */	  
        tpmC = (60.0 * (wf->theCOQueries - complWhenStarted)) / (cur_time - clStartTime); 
        fprintf(stdout, "Queries completed - %d, current throughput - %.2f queries/min\n", (wf->theCOQueries - complWhenStarted), tpmC);

        // sleep for the client specified client time
        sleep(clThinkTime);
    }
  
    free (deck);
    return ((void*)0);
}


#include "engine/namespace.h"
