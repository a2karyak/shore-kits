/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _COMMAND_HANDLER_H
#define _COMMAND_HANDLER_H



class command_handler_t {

public:

    virtual void init()=0;
    virtual void handle(const char* command)=0;

};



#endif
