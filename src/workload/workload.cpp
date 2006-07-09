// -*- mode:C++; c-basic-offset:4 -*-

/** @file    : qpipe_wl.cpp
 *  @brief   : Workload related classes
 *  @version : 0.1
 *  @history :
 6/11/2006: Initial version
*/ 

/*  Includes the implementation of the classes:
    workload_factory
    workload_t
    client_counter
    client_t
*/

#include "workload/workload.h"


// include me last!!!
#include "engine/namespace.h"



/* Number of concurrent clients */
const size_t STD_NUM_CLIENTS = 4;

/* Thinking time for each query */
const size_t STD_THINK_TIME = 0;

/* Number of queries executed by each client */
const size_t STD_NUM_QUERIES = 10;

/* Selected query mix executed by each client */
const int STD_SEL_QUERY = 0;



////////////////////////////////////
// methods for workload_factory

workload_factory* workload_factory::workloadFactoryInstance = NULL;
pthread_mutex_t workload_factory::instance_mutex = PTHREAD_MUTEX_INITIALIZER;



/** @fn    : instance()
 *  @brief : Returns the unique instance of the TPC-H workload factory class
 */

workload_factory* workload_factory::instance() {

    // first check without concern of locking
    if ( workloadFactoryInstance == NULL ) {

        // instance does not exist yet
        pthread_mutex_lock(&instance_mutex);

        // we have the lock but another thread may have gotten the lock first
        if (workloadFactoryInstance == NULL) {
            workloadFactoryInstance = new workload_factory();
        }

        // release instance mutex
        pthread_mutex_unlock(&instance_mutex);
    }

    return(workloadFactoryInstance);
}



/** @fn    : Constructor
 *  @brief : Starts the parser thread 
 */ 

workload_factory::workload_factory() {

    /* initialize status variables */
    isInteractive = 1; // by default is interactive 
    
    /* initialize the workload creator mutex */
    pthread_mutex_init(&theWLCreation_mutex, NULL);

    /* initialize the counter and the handles of the workloads */
    pthread_mutex_lock(&theWLCreation_mutex);
    theWLCounter = 0;
    theWLs = new workload_t*[MAX_WORKLOADS];
    theWLsHandles = new pthread_t[MAX_WORKLOADS];
    theWLsStatus = new int[MAX_WORKLOADS];
    memset(theWLsStatus, 0, sizeof(int)*MAX_WORKLOADS);
    pthread_mutex_unlock(&theWLCreation_mutex);

    /* initialize the stats mutex */
    pthread_mutex_init(&theStat_mutex, NULL);

    /* initialize the global completed queries counter */
    pthread_mutex_lock(&theStat_mutex);
    theCOQueries = 0;
    pthread_mutex_unlock(&theStat_mutex);


    /* create the parser */
    theParser = new parser_t();
}



/** @fn    : Destructor
 *  @brief : Joins the parser thread handle, and destroys the mutex
 */

workload_factory::~workload_factory() {

    TRACE(TRACE_DEBUG, "Workload Factory Destructor\n");

    // joins to all the created workloads
    pthread_mutex_lock(&theWLCreation_mutex);
    int i = 0;

    TRACE( TRACE_DEBUG, "Deleting theWLs array");
    if (theWLs) {

        for (i=0;i<MAX_WORKLOADS;i++) {
            if (theWLs[i]) {
                TRACE( TRACE_DEBUG, "Deleting theWLs[%d]", i);
                delete theWLs[i];
                theWLs[i] = NULL;
            }
        }

        delete [] theWLs;
        theWLs = NULL;
    }


    TRACE( TRACE_DEBUG, "Deleting theWLsHandles array");
    if (theWLsHandles) {

        TRACE( TRACE_DEBUG, "Joining the created Workloads %s", theWLCounter);
        for (i=0;i<theWLCounter;i++) {
            pthread_join(theWLsHandles[i], NULL);
        }

        delete [] theWLsHandles;
        theWLsHandles = NULL;
    }


    pthread_mutex_unlock(&theWLCreation_mutex);

    // destroys the workload creation mutex
    pthread_mutex_destroy(&theWLCreation_mutex);

    // destroys the instance mutex
    pthread_mutex_destroy(&workload_factory::instance_mutex);
}



/** @fn     : attach_client(client_t)
 *  @brief  : Creates a new workload consisting only of one client, and waits if not interactive
 *  @return : 0 on success, -1 otherwise.
 */ 

int workload_factory::attach_client(client_t new_client) {

    return( attach_clients(1, new_client));
}



/** @fn     : attach_clients(int, client_t)
 *  @brief  : Adds a new workload consisting of  N new clients, and waits if not interactive
 *  @return : 0 on success, -1 otherwise.
 */ 

int workload_factory::attach_clients(int noClients, client_t templ) {

    pthread_mutex_lock(&theWLCreation_mutex);

    if (theWLCounter + 1 >= MAX_WORKLOADS) {
        TRACE( TRACE_ALWAYS,
               "Cannot have more than %d workloads. Aborting client attach...\n",
               MAX_WORKLOADS);

        pthread_mutex_unlock(&theWLCreation_mutex);

        return (-1);
    }

    if (noClients <= 0) {
        TRACE( TRACE_ALWAYS,
               "Cannot create negative number of clients %d. Aborting client attach...\n",
               noClients);
        
        pthread_mutex_unlock(&theWLCreation_mutex);
    
        return (-1);
    }

    if (&templ == NULL) {
        TRACE( TRACE_ALWAYS, "Illegal client template. Aborting client attach...\n");
        pthread_mutex_unlock(&theWLCreation_mutex);

        return (-1);
    }

    TRACE( TRACE_ALWAYS, "ADDING %d CLIENTS FOR QUERY: %d. ITERATIONS: %d\n",
           noClients, templ.get_sel_query(), templ.get_num_iter());
  
    // creates a new workload
    theWLs[theWLCounter] = new workload_t("WORKLOAD-%d", theWLCounter);

    // sets the workload parameters
    theWLs[theWLCounter]->set_run(noClients, templ.get_think_time(),
                                  templ.get_num_iter(), templ.get_sel_query(),
                                  templ.get_sel_sql());

    // gets the index of the newly created workload
    theWLs[theWLCounter]->set_idx( theWLCounter );

    /* starts the workload thread */
    if ( thread_create( &theWLsHandles[theWLCounter], theWLs[theWLCounter]) ) {
	TRACE(TRACE_ALWAYS, "thread_create failed\n");
	QPIPE_PANIC();
    }

    // in case of not interactive copy the current WLCounter
    int copyWLCounter = theWLCounter;
    
    // increases the workload counter
    theWLCounter++;
    
    // releases the lock
    pthread_mutex_unlock(&theWLCreation_mutex);

    // if not interactive join the workload thread
    if (!isInteractive) {
        TRACE( TRACE_DEBUG, "Not Interactive mode joining workload %d thread\n", copyWLCounter);
        void *wl_ret;
        pthread_join(theWLsHandles[copyWLCounter], &wl_ret);
        TRACE( TRACE_DEBUG, "workload %d joined\n", copyWLCounter);
    }
    
    
    
    return(0);
}



/** @fn    : create_wl(const int, const int, const int, const int, const string)
 *  @brief : Creates a workload and returns its index, it does not start the thread
 */

int workload_factory::create_wl(const int noClients, const int thinkTime,
                                const int noCompleted,const int selQuery,
                                const string selSQL) {
  
    int wl_idx = -1;

    pthread_mutex_lock(&theWLCreation_mutex);

    TRACE( TRACE_DEBUG, "Creating workload %d\n", theWLCounter);

    // creates a new workload
    theWLs[theWLCounter] = new workload_t("WORKLOAD-%d", theWLCounter);

    // sets the workload parameters
    theWLs[theWLCounter]->set_run(noClients, thinkTime, noCompleted,
                                  selQuery, selSQL);

    // gets the index of the newly created workload
    wl_idx = theWLCounter;
    theWLs[theWLCounter]->set_idx( theWLCounter );

    // increases the workload counter
    theWLCounter++;

    print_info();

    pthread_mutex_unlock(&theWLCreation_mutex);

    return (wl_idx);
}



/** @fn    : print_info()
 *  @brief : Prints information about the factory
 */

void workload_factory::print_info() {

    TRACE( TRACE_DEBUG, "Started workloads: %d\n", theWLCounter);
}



/** @fn    : print_wls_info()
 *  @brief : Prints information for each started workload
 */

void workload_factory::print_wls_info() {

    for (int i=0;i<theWLCounter;i++) {
        if (theWLsStatus[i]) {
            theWLs[i]->get_info(1);
        }
    }
}



/** @fn    : update_wl_status(int, int)
 *  @brief : Updates the entry iWLIdx of the array that contains the status of each
 *           workload.  If the array contains only completed workloads it resets
 *           the value of the theWLCounter
 */

void workload_factory::update_wl_status(int iWLIdx, int iStatus) {
  
    // check the index value
    if ((iWLIdx < 0) || (iWLIdx >= MAX_WORKLOADS)) {
        TRACE( TRACE_ALWAYS, "Illegal Workloads index value %d\n", iWLIdx);
    }
    else {
        pthread_mutex_lock(&theStat_mutex);
        theWLsStatus[iWLIdx] = iStatus;
        pthread_mutex_unlock(&theStat_mutex);
    }

    // check if all workloads have finished
    for (int i=0;i<MAX_WORKLOADS;i++) {

        // if any workload is active
        if (theWLsStatus[i] == 1) {
            TRACE( TRACE_DEBUG, "Workload %d is still active\n", i);
            return;
        }
    }

    // if we reach this point no workload is active
    TRACE( TRACE_DEBUG, "Factory: No workload is active...\nFactory: Reseting...\n");
    theWLCounter = 0;

    // we check the execution mode and if not Interactive we exit
    if (!isInteractive) {
        TRACE( TRACE_DEBUG, "Executing in Batch mode.\tShould Exit...\n");
        // exit(0);
    }

    print_wls_info();
}



///////////////////////////////
// methods for workload_t


/** @fn    : init(const char*, va_list)
 *  @brief : Creates the mutex  and initializes the counters
 */

void workload_t::init(const char* format, va_list ap) {

    // set thread name
    init_thread_name_v(format, ap);

    // member variables
    wlNumClients = STD_NUM_CLIENTS;
    wlThinkTime = STD_THINK_TIME;
    wlNumQueries = STD_NUM_QUERIES;
    wlSelQuery = STD_SEL_QUERY;

    // initialization of mutex
    pthread_mutex_init(&workload_mutex, NULL);

    pthread_mutex_lock(&workload_mutex);
    queriesCompleted = 0;
    wlStarted = 0;
    pthread_mutex_unlock(&workload_mutex);

    myIdx = -1;
}



/** @fn    : Constructor
 *  @brief : Formats the thread name and initiliazes member variables
 */

workload_t::workload_t(const char* format, ...) {

    // standard initialization
    va_list ap;
    va_start(ap, format);
    init(format, ap);
    va_end(ap);
}



/** @fn    : Destructor
 *  @brief : Deletes all the arrays and destroys the mutex
 */

workload_t::~workload_t() {

    //  TRACE_DEBUG("Destructor Deleting clientHandles\n");
    if (clientHandles) {
        delete [] clientHandles;
        clientHandles = NULL;
    }
    
    //  TRACE_DEBUG("Destructor Deleting runningClients\n");
    if (runningClients) {
        delete [] runningClients;
        runningClients = NULL;
    }

    pthread_mutex_destroy(&workload_mutex);
    
    // notify factory
    TRACE( TRACE_DEBUG, "Notifying Factory Idx=%d\tStatus=0\n", myIdx);
    workload_factory* wf = workload_factory::instance();
    wf->update_wl_status(myIdx, 0);
}



/** @fn     : set_run()
 *  @brief  : Sets the initial workload parameters, if they are valid
 *  @return : 0 on success, -1 otherwise.
 */

int workload_t::set_run(const int noClients, const int thinkTime, const int noQueries,
                        const int selQuery, const string selSQL) {

    if ((noClients > 0) && (noClients < MAX_CLIENTS)) {
        wlNumClients = noClients;
    }
    else {
        TRACE(TRACE_DEBUG, "Invalid number of clients %d\n", noClients);
        return (-1);
    }

    if (!(thinkTime<0)) {
        wlThinkTime = thinkTime;
    }
    else {
        TRACE(TRACE_DEBUG, "Invalid think time interval %d\n", thinkTime);
        return (-1);
    }

    if (noQueries > 0) {
        wlNumQueries = noQueries;
    }
    else {
        TRACE(TRACE_DEBUG, "Invalid number of queries %d\n", noQueries);
        return (-1);
    }

    /**
     * @TODO: Should check with the query_registart about the query number
     */

    /*
      if ((selQuery >= 0) && (selQuery < 23)) {
      TRACE(TRACE_DEBUG, "Invalid query number %d\n", selQuery);
      initialSelectedQuery = selQuery;
      }
    */
    wlSelQuery = selQuery;
        
    if (selSQL.size() > 0) {
        wlSQLText = selSQL;
        TRACE( TRACE_DEBUG, "Setting workload SQL = %s\n", wlSQLText.c_str());
    }

    /* If reach this point, return success */
    return(0);
}



/** @fn     : run()
 *  @brief  : Initializes the corresponding threads of clients, starts them, and then joins them.
 *            It will return only when all the clients have their requests completed.
 */

void* workload_t::run() {

    TRACE( TRACE_DEBUG, "Workload %d starts running\n", myIdx);
    
    // return immediately if workload already started
    pthread_mutex_lock(&workload_mutex);

    if (wlStarted) {
        TRACE( TRACE_ALWAYS, "Workload has already started TH=%x\n", pthread_self());
        pthread_mutex_unlock(&workload_mutex);
        return ((void*)1);
    }
    else {
        wlStarted = 1;

        TRACE( TRACE_DEBUG, "Notifying Factory Idx=%d\tStatus=%d\n", myIdx, wlStarted);
        workload_factory* wf = workload_factory::instance();
        wf->update_wl_status(myIdx, wlStarted);
    }

    pthread_mutex_unlock(&workload_mutex);

  
    runningClients = new client_t*[wlNumClients];
    clientHandles = new pthread_t[wlNumClients];

    // print general info for the workload
    get_info(0);
  
    // record the starting time of the experiment
    wlStartTime = time(&wlStartTime);
  
    // create and start the clients
    int i;
    for (i = 0; i < wlNumClients; i++) {

        TRACE( TRACE_DEBUG, "Creating client %d of %d\n", (i+1), wlNumClients);

        runningClients[i] = new client_t(wlSelQuery, wlThinkTime, wlNumQueries, "WORK-%d-CL-%d", myIdx, i);
        if (wlSQLText.size() > 0) {
            runningClients[i]->set_sql(wlSQLText);
        }
        runningClients[i]->set_workload(this);
    
        TRACE( TRACE_DEBUG, "Starting client %d of %d\n", i, wlNumClients);

        
        /* starts client thread */
        if ( thread_create( &clientHandles[i], runningClients[i]) ) {
            TRACE(TRACE_ALWAYS, "thread_create failed\n");
            QPIPE_PANIC();
        }
    
        // sleeps for 1 usec between the creation of the client threads
        usleep(1);
    }

    // now wait for all clients to complete
    for (i = 0; i < wlNumClients; ++i) {
        pthread_join(clientHandles[i], NULL);
    }

    // THE WORKLOAD HAS FINISHED
    TRACE( TRACE_DEBUG, "Workload has finished\n");
    
    // update isStarted flag
    wlStarted = 0;

    // record the ending time of the experiment
    wlEndTime = time(&wlEndTime);

    // final end of workload info
    get_info(1);

    return ((void*)0);
}



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
        float tpmC = (3600.0 * queriesCompleted) / (wlEndTime - wlStartTime);
        
        fprintf(stdout, "~~~\n");
        fprintf(stdout, "~~~ Clients           = %d \n", wlNumClients);
        fprintf(stdout, "~~~ Think Time        = %d \n", wlThinkTime);
        fprintf(stdout, "~~~ Iterations        = %d \n", wlNumQueries);
        fprintf(stdout, "~~~ Query             = %d \n", wlSelQuery);  
        fprintf(stdout, "~~~ Completed Queries = %d \n", queriesCompleted);
        fprintf(stdout, "~~~ Duration          = %ld \n", (wlEndTime - wlStartTime));
        fprintf(stdout, "~~~\n");
        fprintf(stdout, "~~~ Throughput        = %.2f queries/hour \n", tpmC);
        fprintf(stdout, "~~~\n");
    }
}



/** @fn    : set_idx(int)
 *  @brief : Sets the reference to the index in the factory array of workloads for this workload
 */

void workload_t::set_idx(int factoryWLIdx) {

    // makes a simple check of the parameter and sets the index
    if ((factoryWLIdx < 0) || (factoryWLIdx > MAX_WORKLOADS) ) {
        TRACE( TRACE_ALWAYS, "Illegal Factory index: %d\n", factoryWLIdx);
    }
    else {
        myIdx = factoryWLIdx;
    }
}


#include "engine/namespace.h"
