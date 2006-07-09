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
#include <sys/time.h>
#include <math.h>

/* QPipe Engine */
#include "trace.h"
#include "qpipe_panic.h"
#include "engine/util/static_hash_map_struct.h"
#include "engine/util/time_util.h"


// include me last!!!
#include "engine/namespace.h"

using std::string;


#define GEN_CMD "gen_job"
#define GEN_CMD_DESC "Generic Job"



/**
 * class job_t
 *
 * @brief: Class that provides the API for the coded QPipe queries
 */

class job_t {
protected:

    /* Used for job definition */
    string job_cmd;
    string job_desc;

    /* Used for the random predicates */
    struct timeval tv;
    uint mv;
    
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
    
    
    /* Initialize */
    virtual void init() {
        TRACE( TRACE_DEBUG, "Initializing Empty Job. id=%s\n", job_cmd.c_str());
    }
    
    /* Print predicates */
    virtual void print_predicates() {
        TRACE( TRACE_DEBUG, "Printing Job Predicates\n");
    }
    
    /* Start executing */
    virtual void* start() {
        TRACE( TRACE_ALWAYS, "Executing Empty Job. id=%s\n", job_cmd.c_str());

        init();

        print_predicates();
        
        return ((void*)0);
    }
};




/**
 * class job_driver_t
 *
 * @brief: Class that acts as a driver for Jobs
 */

class job_driver_t {
public:
    job_driver_t() { }

    virtual ~job_driver_t() { }

    virtual void* drive( ) { return ((void*)0); }
    virtual void print_info( ) { }    
};



#include "engine/namespace.h"

#endif // __JOB_H

