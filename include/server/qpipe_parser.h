// -*- mode:C++; c-basic-offset:4 -*-

/** @file    : qpipe_parser.h
 *  @brief   : Parser of SQL and QPipe commands
 *  @version : 0.1
 *  @history :
 6/11/2006: Initial version
*/ 

# ifndef __QPIPE_PARSER_H
# define __QPIPE_PARSER_H


/* Standard */
#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <cstdlib>
#include <string>

/* QPipe Engine */
#include "thread.h"
#include "trace/trace.h"
#include "qpipe_panic.h"

/* QPipe Server */
#include "qpipe_parser.h"
#include "qpipe_wl.h"

using namespace qpipe;

using std::string;

/**
 * class parser_t
 * 
 * @brief: Class representing the parser. It knows the format of the
 *         received messages, parses them, and acts accordingly.
 */

class parser_t {

 private:
    
    /* handlers */
    void handle_msg_workload(char* cmd);
    void handle_msg_sql(char* cmd);
    void handle_msg_workload_info(char* cmd);
    
 public:

    parser_t() {
        TRACE( TRACE_DEBUG, "Parser started\n");
    }
    
    ~parser_t() {
        TRACE( TRACE_DEBUG, "Parser ended\n");
    }

    /* Parse command */
    int parse(string std_cmd);

    /* usage information */
    void print_usage(FILE* out_stream);
};


# endif // __QPIPE_PARSER_H
