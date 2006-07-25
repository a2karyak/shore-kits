/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _TPCH_HANDLER_H
#define _TPCH_HANDLER_H

#include "engine/thread.h"
#include "engine/util/c_str.h"
#include "engine/dispatcher/dispatcher_policy.h"
#include "server/command/command_handler.h"
#include "workload/driver.h"
#include "workload/workload.h"
#include <map>

using std::map;
using qpipe::driver_t;



class tpch_handler_t : public command_handler_t {

    
    // database fields
    enum state_t {
        TPCH_HANDLER_UNINITIALIZED = 0,
        TPCH_HANDLER_INITIALIZED,
        TPCH_HANDLER_SHUTDOWN
    };
    
    static pthread_mutex_t state_mutex;
    static state_t         state;

    
    // command fields
    map<c_str, driver_t*> _drivers;
    map<c_str, dispatcher_policy_t*> _dispatcher_policies;
    

    void add_driver(const char*  tag, driver_t* driver);
    void add_driver(const c_str &tag, driver_t* driver);
    
    void add_dispatcher_policy(const char*  tag, dispatcher_policy_t* dp);
    void add_dispatcher_policy(const c_str &tag, dispatcher_policy_t* dp);
    
    void print_run_statistics(workload_t::results_t &results);

public:

    virtual void init();
    virtual void handle_command(const char* command);
    virtual void shutdown();

    virtual ~tpch_handler_t() { }
};



#endif
