/* -*- mode:C++; c-basic-offset:4 -*- */

#include "core/packet.h"
#include "util.h"
#include "server/command/sharing.h"
#include "server/print.h"
#include "server/config.h"

#include <cstring>
#include <cstdio>



/* definitions of exported methods */

void sharing_t::init()
{

};



void sharing_t::handle_command(const char* command)
{

    char command_tag[SERVER_COMMAND_BUFFER_SIZE];
    char tag[SERVER_COMMAND_BUFFER_SIZE];


    // parse command tag (should be something like "sharing")
    if ( sscanf(command, "%s", command_tag) < 1 ) {
        TRACE(TRACE_ALWAYS, "Unable to parse command tag!\n");
        print_usage(command_tag);
        return;
    }
    

    // we can now use command tag for all messages...


    // parse tag
    if ( sscanf(command, "%*s %s", tag) < 1 ) {
        print_usage(command_tag);
        return;
    }


    if (!strcmp(tag, "global_enable")) {
        global_enable();
        return;
    }

    
    if (!strcmp(tag, "global_disable")) {
        global_disable();
        return;
    }

    if (!strcmp(tag, "is_global_enabled")) {
        TRACE(TRACE_ALWAYS, "Global scheduling policy: %s\n",
              global_sharing_policy().data());
        return;
    }

    
    TRACE(TRACE_ALWAYS, "Unrecognized tag %s\n", tag);
    print_usage(command_tag);
}



void sharing_t::global_enable()
{
    qpipe::packet_t::global_osp_set(true);
}



void sharing_t::global_disable()
{
    qpipe::packet_t::global_osp_set(false);
}



c_str sharing_t::global_sharing_policy()
{
    if (qpipe::packet_t::global_osp_is_enabled())
        return c_str("OSP_FULL");
    else
        return c_str("OSP_NONE");
}



void sharing_t::print_usage(const char* command_tag) {
    TRACE(TRACE_ALWAYS, "%s global_enable|global_disable|is_global_enabled\n", command_tag);
}
