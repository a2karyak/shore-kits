/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _COMMAND_HANDLER_H
#define _COMMAND_HANDLER_H



class command_handler_t {

public:
    
    virtual void init() {
        // default implementation ... does nothing
    }

    virtual void handle_command(const char* command)=0;
    
    virtual void shutdown() {
        // default implementation ... does nothing        
    }

    virtual ~command_handler_t() { }
};



#endif
