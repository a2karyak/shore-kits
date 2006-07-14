/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file    : job_directory.h
 *  @brief   : Singleton class that is the Job directory
 *  @version : 0.1
 *  @history :
 7/7/2006: Initial version
*/ 

#ifndef __JOB_DIRECTORY_H
#define __JOB_DIRECTORY_H

#include "workload/job.h"
#include "engine/util/c_str.h"



// include me last!!!
#include "engine/namespace.h"


#define JOB_DIRECTORY_HASH_MAP_BUCKETS 64


/**
 * class job_directory
 *
 * @brief: Singleton class that contains a hash table with identifiers
 *         to job and pointer to them
 */

class job_directory {
private:

    /* instance and mutex */
    static job_directory* jobDirInstance;
    static pthread_mutex_t instance_mutex;

    /* registered jobs directory */
    struct static_hash_map_s jobs_directory;
    struct static_hash_node_s jobs_directory_buckets[JOB_DIRECTORY_HASH_MAP_BUCKETS];
    pthread_mutex_t map_mutex;    
    
    /* stats */
    int jobCounter;
    pthread_mutex_t stats_mutex;
    
    job_directory();
    ~job_directory();
    
public:
    /* instance call */
    static job_directory* instance();

    /* register a job */
    int _register_job_driver(const c_str &sJobCmd, job_driver_t* aJobDriver);

    /* print info about the registered job drivers */
    void _print_info();

    /* return the correct job driver */
    job_driver_t* _get_job_driver(const c_str &sJobCmd);
    
    /* static wrapper functions */
    static int register_job_driver(const char* sJobCmd, job_driver_t* aJobDriver);
    static int unregister_job_driver(const char* sJobCmd);
    static void print_info();
    static job_driver_t* get_job_driver(const char* sJobCmd);
};



#include "engine/namespace.h"

#endif // __JOB_DIRECTORY_H
