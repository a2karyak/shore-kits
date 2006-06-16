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
#include <cassert>

/* QPipe Engine */
#include "trace/trace.h"
#include "qpipe_panic.h"



// include me last!!!
#include "namespace.h"

using std::string;

#define GEN_CMD "gen_job"


class Job;


/**
 * class job_repos
 *
 * @brief: Singleton class that contains a hash table with identifiers to job and pointer to them
 */

class job_repos {
private:
    job_repos() { }
    ~job_repos() { }
    
public:
    job_repos* instance;
    
};



/**
 * class job_t
 *
 * @brief: Class that provides the API for the coded QPipe queries
 */


class job_t {
private:
    string job_cmd;
    
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
 


#include "namespace.h"

#endif // __JOB_H

