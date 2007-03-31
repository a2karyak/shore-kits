/* -*- mode:C++; c-basic-offset:4 -*- */


#include "util.h"
#include "server/command/tracer.h"
#include "server/print.h"
#include "server/config.h"

#include <cstring>
#include <cstdio>



/* definitions of exported methods */

void tracer_t::init()
{

#define ADD_TYPE(x) _known_types[c_str("%s", #x)] = x;
    
    ADD_TYPE(TRACE_ALWAYS);
    ADD_TYPE(TRACE_TUPLE_FLOW);
    ADD_TYPE(TRACE_PACKET_FLOW);
    ADD_TYPE(TRACE_SYNC_COND);
    ADD_TYPE(TRACE_SYNC_LOCK);
    ADD_TYPE(TRACE_THREAD_LIFE_CYCLE);
    ADD_TYPE(TRACE_TEMP_FILE);
    ADD_TYPE(TRACE_CPU_BINDING);
    ADD_TYPE(TRACE_QUERY_RESULTS);
    ADD_TYPE(TRACE_RESPONSE_TIME);
    ADD_TYPE(TRACE_WORK_SHARING);
    ADD_TYPE(TRACE_TUPLE_FIFO_FILE);

};



void tracer_t::handle_command(const char* command)
{

    char command_tag[SERVER_COMMAND_BUFFER_SIZE];
    char tag[SERVER_COMMAND_BUFFER_SIZE];


    // parse command tag (should be something like "tracer")
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


    if (!strcmp(tag, "known")) {
        print_known_types();
        return;
    }


    if (!strcmp(tag, "list")) {
        print_enabled_types();
        return;
    }


    if (!strcmp(tag, "enable")) {
        char trace_type[SERVER_COMMAND_BUFFER_SIZE];
        if ( sscanf(command, "%*s %*s %s", trace_type) < 1 ) {
            print_usage(command_tag);
            return;
        }
        enable(trace_type);
        return;
    }

    
    if (!strcmp(tag, "disable")) {
        char trace_type[SERVER_COMMAND_BUFFER_SIZE];
        if ( sscanf(command, "%*s %*s %s", trace_type) < 1 ) {
            print_usage(command_tag);
            return;
        }
        disable(trace_type);
        return;
    }

    TRACE(TRACE_ALWAYS, "Unrecognized tag %s\n", tag);
    print_usage(command_tag);
}



void tracer_t::enable(const char* type)
{
    map<c_str, int>::iterator it;
    for (it = _known_types.begin(); it != _known_types.end(); ++it) {
        if (!strcmp(it->first.data(), type)) {
            /* found it! */
            int mask = it->second;
            TRACE_SET(TRACE_GET() | mask);
            TRACE(TRACE_ALWAYS, "Enabled %s\n", it->first.data());
            return;
        }
    }

    TRACE(TRACE_ALWAYS, "Unknown type %s\n", type);
}



void tracer_t::disable(const char* type)
{
    map<c_str, int>::iterator it;
    for (it = _known_types.begin(); it != _known_types.end(); ++it) {
        if (!strcmp(it->first.data(), type)) {
            /* found it! */
            int mask = it->second;
            TRACE_SET(TRACE_GET() & (~mask));
            TRACE(TRACE_ALWAYS, "Disabled %s\n", it->first.data());
            return;
        }
    }

    TRACE(TRACE_ALWAYS, "Unknown type %s\n", type);
}



void tracer_t::print_known_types() {

    map<c_str, int>::iterator it;
    for (it = _known_types.begin(); it != _known_types.end(); ++it) {
        TRACE(TRACE_ALWAYS,
              "Registered trace type %s\n", it->first.data());
    }
}



void tracer_t::print_enabled_types() {

    map<c_str, int>::iterator it;
    for (it = _known_types.begin(); it != _known_types.end(); ++it) {
        int mask = it->second;
        if ( TRACE_GET() & mask )
            TRACE(TRACE_ALWAYS, "Enabled type %s\n", it->first.data());
    }
}



void tracer_t::print_usage(const char* command_tag) {
    TRACE(TRACE_ALWAYS, "%s known|list|enable <type>|disable <type>\n", command_tag);
}
