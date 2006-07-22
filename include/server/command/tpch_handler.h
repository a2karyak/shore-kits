/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _TPCH_HANDLER_H
#define _TPCH_HANDLER_H

#include "engine/thread.h"
#include "server/command/command_handler.h"



class tpch_handler_t : public command_handler_t {

    enum state_t {
        TPCH_HANDLER_UNINITIALIZED = 0,
        TPCH_HANDLER_INITIALIZED,
        TPCH_HANDLER_SHUTDOWN
    };
    
    static pthread_mutex_t state_mutex;
    static state_t         state;
    
public:

    virtual void init();
    virtual void handle_command(const char* command);
    virtual void shutdown();

    virtual ~tpch_handler_t() { }
};



#endif
