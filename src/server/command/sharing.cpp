/* -*- mode:C++; c-basic-offset:4 -*- */

#include "core/packet.h"
#include "util.h"
#include "server/command/sharing.h"
#include "server/print.h"
#include "server/config.h"

#include "vldb07_model/model.h"

#include <cstring>


using namespace VLDB07Model;


/* definitions of exported methods */

void sharing_t::init()
{

};



void sharing_t::handle_command(const char* command)
{

    char command_tag[SERVER_COMMAND_BUFFER_SIZE];
    char tag[SERVER_COMMAND_BUFFER_SIZE];
    char option_tag[SERVER_COMMAND_BUFFER_SIZE];


    // parse command tag (should be something like "sharing")
    if ( sscanf(command, "%s", command_tag) < 1 ) {
        TRACE(TRACE_ALWAYS, "Unable to parse command tag!\n");
        print_usage(command_tag);
        return;
    }
    

    // we can now use command tag for all messages...


    // parse tag
    if ( sscanf(command, "%*s %s %s", tag, option_tag) < 2 ) {
        print_usage(command_tag);
        return;
    }


    if (!strcmp(tag, "global")) {
        if (!strcmp(option_tag, "enable")) {
            global_enable();
            return;
        }
        if (!strcmp(option_tag, "disable")) {
            global_disable();
            return;
        }
        if (!strcmp(option_tag, "report")) {
            TRACE(TRACE_ALWAYS, "Global scheduling policy: %s\n",
                  global_sharing_policy().data());
            return;
        }
        print_usage(command_tag);
        return;
    }

    if (!strcmp(tag, "vldb07_model")) {
        if (!strcmp(option_tag, "enable")) {
            global_vldb07_model_enable();
            return;
        }
        if (!strcmp(option_tag, "disable")) {
            global_vldb07_model_disable();
            return;
        }
        if (!strcmp(option_tag, "report")) {
            TRACE(TRACE_ALWAYS, "Global VLDB07 model policy: %s\n",
                  global_vldb07_model_policy().data());
            return;
        }
        print_usage(command_tag);
        return;
    }

    if ((!strcmp(tag, "stage_on")) || (!strcmp(tag, "stage_off"))) {
        bool val = !strcmp(tag, "stage_on");
        if (   (!strcmp(option_tag, "TSCAN")) 
            || (!strcmp(option_tag, "AGGREGATE")) 
            || (!strcmp(option_tag, "PARTIAL_AGGREGATE")) 
            || (!strcmp(option_tag, "HASH_AGGREGATE")) 
            || (!strcmp(option_tag, "HASH_JOIN")) 
            || (!strcmp(option_tag, "PIPE_HASH_JOIN")) 
            || (!strcmp(option_tag, "FUNC_CALL")) 
            || (!strcmp(option_tag, "SORT")) 
            || (!strcmp(option_tag, "SORTED_IN")) 
            || (!strcmp(option_tag, "ECHO")) 
            || (!strcmp(option_tag, "SIEVE")) 
           )
        {
            c_str action(option_tag);
            qpipe::set_osp_for_type(c_str(action), val);
            return;
        }
        if (!strcmp(option_tag, "all")) {
            qpipe::set_osp_for_type(c_str("TSCAN"), val);
            qpipe::set_osp_for_type(c_str("AGGREGATE"), val);
            qpipe::set_osp_for_type(c_str("PARTIAL_AGGREGATE"), val);
            qpipe::set_osp_for_type(c_str("HASH_AGGREGATE"), val);
            qpipe::set_osp_for_type(c_str("HASH_JOIN"), val);
            qpipe::set_osp_for_type(c_str("PIPE_HASH_JOIN"), val);
            qpipe::set_osp_for_type(c_str("FUNC_CALL"), val);
            qpipe::set_osp_for_type(c_str("SORT"), val);
            qpipe::set_osp_for_type(c_str("SORTED_IN"), val);
            qpipe::set_osp_for_type(c_str("ECHO"), val);
            qpipe::set_osp_for_type(c_str("SIEVE"), val);
            return;
        }
        print_usage(command_tag);
        return;
    }

    TRACE(TRACE_ALWAYS, "Unrecognized tag %s\n", tag);
    print_usage(command_tag);
}


void sharing_t::global_enable()
{
    qpipe::osp_global_policy = qpipe::OSP_FULL;
    VLDB07_model_t::set_all_packet_flags(true);
    fprintf(stderr, "************* set all flags to true\n");
}



void sharing_t::global_disable()
{
    qpipe::osp_global_policy = qpipe::OSP_NONE;
}



c_str sharing_t::global_sharing_policy()
{
    switch(qpipe::osp_global_policy) {
    case qpipe::OSP_NONE:
        return c_str("OSP_NONE");
    case qpipe::OSP_FULL:
        return c_str("OSP_FULL");
    default:
        return c_str("Unrecognized type!");
    }
}



void sharing_t::global_vldb07_model_enable()
{
    VLDB07_model_t::enable_model();
}



void sharing_t::global_vldb07_model_disable()
{
    VLDB07_model_t::disable_model();
}



c_str sharing_t::global_vldb07_model_policy()
{
    switch(VLDB07_model_t::is_model_enabled()) {
      case true:  return c_str("VLDB07_MODEL_ENABLED");
      case false: return c_str("VLDB07_MODEL_DISABLED");
      default:    return c_str("Unrecognized type!");
    }
}



void sharing_t::print_usage(const char* command_tag) {
    TRACE(  TRACE_ALWAYS
          , "\n%s (global| vldb07_model| stage_on| stage_off) (enable| disable| report)\n  or  \n (stage_on| stage_off)\n (SCAN| AGGREGATE| PARTIAL_AGGREGATE| HASH_AGGREGATE| HASH_JOIN| PIPE_HASH_JOIN| FUNC_CALL| SORT| SORTED_IN| ECHO| SIEVE| all)\n"
          , command_tag);
}
