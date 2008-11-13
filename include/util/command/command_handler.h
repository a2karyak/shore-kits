/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   command_handler.h
 *
 *  @brief:  Interface each shell command should implement 
 *
 *  @author: Ippokratis Pandis (ipandis)
 *  @author: Naju Mancheril (ngm)
 */

#ifndef __CMD_HANDLER_H
#define __CMD_HANDLER_H

#ifdef __SUNPRO_CC
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#else
#include <cstdlib>
#include <cstdio>
#include <cstring>
#endif

#include <vector>

#include "util.h"

using namespace std;



// constants
const int SHELL_COMMAND_BUFFER_SIZE = 64;
const int SHELL_NEXT_CONTINUE       = 1;
const int SHELL_NEXT_QUIT           = 2;



class command_handler_t 
{
public:
    typedef vector<string> aliasList;
    typedef aliasList::iterator aliasIt;

protected:
    aliasList _aliases;
    string _name;
public:

    command_handler_t() {         
    }
    virtual ~command_handler_t() { }


    // COMMAND INTERFACE


    // init/close
    virtual void init() { /* default do nothing */ };
    virtual void close() { /* default do nothing */ };

    // by default should return SHELL_NEXT_CONTINUE
    virtual const int handle(const char* cmd)=0; 

    // should push_back() a set of aliases
    // the first one is the basic command name    
    virtual void setaliases()=0;
    string name() { return (_name); }
    aliasList* aliases() { return (&_aliases); }

    // should print usage
    virtual void usage() { /* default do nothing */ };

    // should return short description
    virtual const string desc()=0;

}; // EOF: command_handler_t



#endif /** __CMD_HANDLER_H **/