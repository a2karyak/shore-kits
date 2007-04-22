/* -*- mode:C++; c-basic-offset:4 -*- */


#include "util.h"
#include "server/command/config_command.h"
#include "server/print.h"
#include "server/config.h"

#include "core/tuple_fifo.h"
#include "core/packet.h"
#include "workload/common/predicates.h"

#include <cstring>
#include <cstdio>


using qpipe::FLUSH_TO_DISK_ON_FULL;
using qpipe::USE_DIRECT_IO;
using qpipe::WAIT_FOR_UNSHARED_TUPLE_FIFOS_TO_DRAIN;
using workload::USE_DETERMINISTIC_PREDICATES;
using qpipe::packet_t;
using qpipe::global_osp_enabled;




/* definitions of exported methods */

void config_command_t::init()
{

#define ADD_VAR(x) _known_config_vars[c_str("%s", #x)] = &x;
    
    ADD_VAR(FLUSH_TO_DISK_ON_FULL);
    ADD_VAR(USE_DIRECT_IO);
    ADD_VAR(WAIT_FOR_UNSHARED_TUPLE_FIFOS_TO_DRAIN);
    ADD_VAR(USE_DETERMINISTIC_PREDICATES);

    _known_config_vars[c_str("GLOBAL_OSP")] = &global_osp_enabled;
};



void config_command_t::handle_command(const char* command)
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


    if (!strcmp(tag, "list")) {
        print_known_vars();
        return;
    }


    if (!strcmp(tag, "enabled")) {
        print_enabled_vars();
        return;
    }


    if (!strcmp(tag, "enable")) {
        char var_name[SERVER_COMMAND_BUFFER_SIZE];
        if ( sscanf(command, "%*s %*s %s", var_name) < 1 ) {
            print_usage(command_tag);
            return;
        }
        enable(var_name);
        return;
    }

    
    if (!strcmp(tag, "disable")) {
        char var_name[SERVER_COMMAND_BUFFER_SIZE];
        if ( sscanf(command, "%*s %*s %s", var_name) < 1 ) {
            print_usage(command_tag);
            return;
        }
        disable(var_name);
        return;
    }

    TRACE(TRACE_ALWAYS, "Unrecognized tag %s\n", tag);
    print_usage(command_tag);
}



void config_command_t::enable(const char* type)
{
    map<c_str, bool*>::iterator it;
    for (it = _known_config_vars.begin(); it != _known_config_vars.end(); ++it) {
        if (!strcmp(it->first.data(), type)) {
            /* found it! */
            bool* var = it->second;
            *var = true;
            TRACE(TRACE_ALWAYS, "Enabled %s\n", it->first.data());
            return;
        }
    }

    TRACE(TRACE_ALWAYS, "Unknown type %s\n", type);
}



void config_command_t::disable(const char* type)
{
    map<c_str, bool*>::iterator it;
    for (it = _known_config_vars.begin(); it != _known_config_vars.end(); ++it) {
        if (!strcmp(it->first.data(), type)) {
            /* found it! */
            bool* var = it->second;
            *var = false;
            TRACE(TRACE_ALWAYS, "Disabled %s\n", it->first.data());
            return;
        }
    }

    TRACE(TRACE_ALWAYS, "Unknown type %s\n", type);
}



void config_command_t::print_known_vars() {

    map<c_str, bool*>::iterator it;
    for (it = _known_config_vars.begin(); it != _known_config_vars.end(); ++it) {
        bool* var = it->second;
        TRACE(TRACE_ALWAYS, "%s: %s\n", it->first.data(), *var ? "ENABLED" : "DISABLED");
    }
}



void config_command_t::print_enabled_vars() {

    map<c_str, bool*>::iterator it;
    for (it = _known_config_vars.begin(); it != _known_config_vars.end(); ++it) {
        bool* var = it->second;
        if ( *var )
            TRACE(TRACE_ALWAYS, "%s: ENABLED\n", it->first.data());
    }
}



void config_command_t::print_usage(const char* command_tag) {
    TRACE(TRACE_ALWAYS, "%s list|enabled|enable <type>|disable <type>\n", command_tag);
}
