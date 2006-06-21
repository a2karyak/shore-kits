/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file    : job.h
 *  @brief   : Definition of a generic job (coded query) class and its repository
 *  @version : 0.1
 *  @history :
 6/15/2006: Initial version
*/ 

#ifndef __JOB_H
#define __JOB_H


/* Standard */
#include <unistd.h>
#include <sys/types.h>
#include <cstdlib>
#include <string>
#include <cstdio>

/* QPipe Engine */
#include "trace.h"
#include "qpipe_panic.h"
#include "engine/util/static_hash_map_struct.h"


// include me last!!!
#include "engine/namespace.h"

using std::string;


#define GEN_CMD "gen_job"
#define GEN_CMD_DESC "Generic Job"

#define JOB_REPOSITORY_HASH_MAP_BUCKETS 64


class job_t;


/**
 * class job_repos
 *
 * @brief: Singleton class that contains a hash table with identifiers
 *         to job and pointer to them
 */

class job_repos {
private:

    /* instance and mutex */
    static job_repos* jobReposInstance;
    static pthread_mutex_t instance_mutex;

    /* registered jobs directory */
    struct static_hash_map_s jobs_directory;
    struct static_hash_node_s jobs_directory_buckets[JOB_REPOSITORY_HASH_MAP_BUCKETS];
    pthread_mutex_t map_mutex;    
    
    /* stats */
    int jobCounter;
    pthread_mutex_t stats_mutex;
    
    job_repos();
    ~job_repos();
    
public:
    /* instance call */
    static job_repos* instance();

    /* register a job */
    int _register_job(job_t* aJob);

    /* unregister a job */
    int _unregister_job(job_t* aJob);

    /* print info about the registered jobs */
    void _print_info();

    /* static wrapper functions */
    static int register_job(job_t* aJob);
    static int unregister_job(job_t* aJob);
    static void print_info();
};



/**
 * class job_t
 *
 * @brief: Class that provides the API for the coded QPipe queries
 */

class job_t {
private:
    string job_cmd;
    string job_desc;
    
public:
    /* Construction - Destruction */
    job_t() {
        job_cmd = GEN_CMD;
    }

    job_t(string cmd) {
        job_cmd = cmd;
    }
    
    virtual ~job_t() { }

    /* Access methods */
    void set_cmd(string a_cmd) {
        if (a_cmd.size() > 0)
            job_cmd = a_cmd;
    }

    string get_cmd() { return (job_cmd); }

    void set_desc(string a_desc) {
        if (a_desc.size() > 0)
            job_desc = a_desc;
    }

    string get_desc() { return (job_desc); }
        
    
    /* Register to the workload repository */
    int register_job();

    /* Unregister */
    int unregister_job();    
    
    /* Start executing */
    virtual int start(void* arg) {
        TRACE( TRACE_ALWAYS, "Executing Empty Job. id=%s\n", job_cmd.c_str());
        
        return (1);
    }
};
 


#include "engine/namespace.h"

#endif // __JOB_H

