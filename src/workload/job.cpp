/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file    : job.cpp
 *  @brief   : Implementation of a generic job (coded query) class and a corresponding
 *             repository
 *  @version : 0.1
 *  @history :
 6/15/2006: Initial version
*/

# include "workload/job.h"
#include "engine/util/static_hash_map.h"
#include "engine/util/hash_functions.h"


// include me last!!!
#include "engine/namespace.h"



/* helper functions */
size_t cmd_hash(const void* key);
int string_comparator(const void* key1, const void* key2);
    
///////////////////
// class job_repos
//


job_repos* job_repos::jobReposInstance = NULL;
pthread_mutex_t job_repos::instance_mutex = PTHREAD_MUTEX_INITIALIZER;



/** @fn    : instance()
 *  @brief : Returns the unique instance of the job repository
 */

job_repos* job_repos::instance() {

    // first check without concern of locking
    if ( jobReposInstance == NULL ) {

        // instance does not exist yet
        pthread_mutex_lock(&instance_mutex);

        // we have the lock but another thread may have gotten the lock first
        if (jobReposInstance == NULL) {
            jobReposInstance = new job_repos();
        }

        // release instance mutex
        pthread_mutex_unlock(&instance_mutex);
    }

    return(jobReposInstance);
}




/** @fn     : Constructor
 *  @brief  : Initializes the static hash map
 */

job_repos::job_repos() {

    
    /* initialize the stats */
    pthread_mutex_init(&stats_mutex, NULL);
    pthread_mutex_lock(&stats_mutex);
    jobCounter = 0;
    pthread_mutex_unlock(&stats_mutex);
    
    /* initialize the static hash map */
    static_hash_map_init( &jobs_directory, &jobs_directory_buckets[0],
                          JOB_REPOSITORY_HASH_MAP_BUCKETS,
                          cmd_hash, string_comparator);
}



/** @fn     : Destructor
 *  @brief  : Removes the allocated variables
 */

job_repos::~job_repos() {

    TRACE(TRACE_DEBUG, "job_repos destructor\n");
}



   
/** @fn     : int _register_job(job_t*)
 *  @brief  : Register the job to the workload repository.
 *            The user can invoke the job by calling -j <unique_cmd>
 *  @return : 0 on success, 1 otherwise
 */
 
int job_repos::_register_job(job_t* aJob) {

    /** @TODO: Not thread safe Should use the map_mutex */
    
    TRACE( TRACE_DEBUG, "Registrering Job: %s\n", aJob->get_cmd().c_str());
    
    /* check for duplicates */
    if ( !static_hash_map_find( &jobs_directory,
                                aJob->get_cmd().c_str(),
                                NULL, NULL ) ) {
        TRACE(TRACE_ALWAYS, "Trying to register duplicate job for command %s\n",
              aJob->get_cmd().c_str());
        QPIPE_PANIC();
    }
    
    /* allocate hash node and copy of key string */
    static_hash_node_t node = (static_hash_node_t)malloc(sizeof(*node));
    if ( node == NULL ) {
        TRACE(TRACE_ALWAYS, "malloc() failed on static_hash_node_t\n");
        QPIPE_PANIC();
    }
    
    char* ptcopy;
    if ( asprintf(&ptcopy, "%s", aJob->get_cmd().c_str()) == -1 ) {
        TRACE(TRACE_ALWAYS, "asprintf() failed\n");
        free(node);
        QPIPE_PANIC();
    }
    
    /* add to hash map */
    static_hash_map_insert( &jobs_directory, ptcopy, aJob, node );
  
    return (0);
}



/** @fn     : int _unregister_job(job_t)
 *  @brief  : Removes the corresponding entry in the Jobs repository
 *  @return : 0 on success, 1 otherwise
 */

int job_repos::_unregister_job(job_t* aJob) {

    /** @TODO: Not thread safe Should use the map_mutex */
    
    TRACE( TRACE_DEBUG, "Removing Job: %s\n", aJob->get_cmd().c_str());
    
    return (static_hash_map_remove( &jobs_directory,
                                    aJob->get_cmd().c_str(),
                                    NULL, NULL));
}




/** @fn    : int _print_info()
 *  @brief : Displays info about the registered jobs
 */

void job_repos::_print_info() {

    static_hash_node_s node;
    job_t* job = NULL;
    
    for (int i=0; i < JOB_REPOSITORY_HASH_MAP_BUCKETS; i++) {
        node = jobs_directory_buckets[i];
        job = (job_t*)node.value;
        
        TRACE( TRACE_DEBUG, "%d. Cmd: %s. Desc: %s\n", i,
               job->get_cmd().c_str(),
               job->get_desc().c_str());
    }
}


/* static wrappers */
int job_repos::register_job(job_t* aJob) {

    return (instance()->_register_job(aJob));
}

int job_repos::unregister_job(job_t* aJob) {

    return (instance()->_unregister_job(aJob));
}

void job_repos::print_info() {

    instance()->_print_info();
}



/* definitions of internal helper functions */
 
/** @fn    : int string_comparator(const void*, const void*)
 *  @brief : String comparison function that can be used as argument
 *           in the hash_map structure
 */
 
int string_comparator(const void* key1, const void* key2) {
  const char* str1 = (const char*)key1;
  const char* str2 = (const char*)key2;
  return strcmp(str1, str2);
}



/** @fn    : size_t cmd_hash(const void*)
 *  @brief : Hashing a string function that can be used as argument
 *            in the hash_map stucture
 */
 
size_t cmd_hash(const void* key) {
  const char* str = (const char*)key;
  return (size_t)RSHash(str, strlen(str));
}
 


///////////////////
// class job_t
//

/** @fn     : int register_job(string)
 *  @brief  : Register the job to the workload repository.
 *            The user can invoke the job by calling -j <unique_cmd>
 *  @return : 0 on success, 1 otherwise
 */

int job_t::register_job() {

    TRACE( TRACE_DEBUG, "Registering Job with id=%s\n", job_cmd.c_str());

    /** @TODO: Add command to repository, with a pointer to the start() */
    return (job_repos::register_job(this));
}



/** @fn     : void unregister_job()
 *  @brief  : Unregister from the workload repository, allows dynamic loading/unloading of commands
 *  @return : 0 on success, 1 otherwise
 */

int job_t::unregister_job() {

    TRACE( TRACE_DEBUG, "Unregistering Job with id=%s\n", job_cmd.c_str());

    /** @TODO: Remove command from repository */
    return (job_repos::unregister_job(this));
}



#include "engine/namespace.h"
