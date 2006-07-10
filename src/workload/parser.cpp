// -*- mode:C++; c-basic-offset:4 -*-

/** @file    : qpipe_parser.cpp
 *  @brief   : Implementation of parser of SQL and QPipe commands
 *  @version : 0.1
 *  @history :
 6/11/2006: Initial version
*/ 

# include "workload/parser.h"
#include "workload/dispatcher_globals.h"


// include me last!!!
#include "engine/namespace.h"


/* CMD constants */
#define STD_DELIM " "


#define MSG_WORKLOAD       0
#define MSG_SQL            1
#define MSG_WORKLOAD_INFO  2



#define SETUP_CMD "setup"
#define HELP_CMD "help"
#define TPCH_CMD "tpch"
#define TPCC_CMD "tpcc"
#define QUIT_CMD "quit"
#define Q_CMD "q" 
#define EXIT_CMD "exit"
#define ALL_CMD "all"
#define COMM_CMD "comm"
#define COMM_START_CMD "start"
#define COMM_STOP_CMD "stop"
#define COMM_STUPID "daemon"
#define WORKLOAD_CMD "workload"

#define QUEST_DELIM ";"
#define QUEST_CHAR ';'
#define EQ_DELIM "="
#define MIN_SQL_TEXT 10


//////////////////////
// class parser_t



/** @fn     : parse(string)
 *  @brief  : Parses the passed command and acts accordingly.
 *  @return : 0 if parsing successful, 1 otherwise
 */

int parser_t::parse(string std_cmd) {

    if (!(std_cmd.size() > 0)) {
        TRACE( TRACE_DEBUG, "Empty command\n");
        return (1);
    }
    
    // copy std_cmd
    char* cmd = (char*)malloc(std_cmd.size()*sizeof(char));
    strncpy(cmd, std_cmd.c_str(), std_cmd.size());
    
    TRACE( TRACE_DEBUG, "Parsing message: [%s]\n", cmd);

    char* tmpbuf = NULL;
    
    // gets msg_type and msg_cmd
    char* msg_type = strtok_r(cmd, STD_DELIM, &tmpbuf);
    
    TRACE( TRACE_DEBUG, "TYPE = %s\tCMD = %s\n", msg_type, tmpbuf);

    switch (atoi(msg_type)) {

    case (MSG_WORKLOAD):
        handle_msg_workload(tmpbuf);
        break;

    case (MSG_SQL):
        handle_msg_sql(tmpbuf);
        break;

    case (MSG_WORKLOAD_INFO):
        handle_msg_workload_info(tmpbuf);
        break;

    default:
        TRACE( TRACE_DEBUG, "+ NOT UNDERSTOOD COMMAND: %s\n", tmpbuf);
        return (1);
    }

    free (cmd);
    return (0);
}



/** @fn     : handle_msg_workload(char*)
 *  @brief  : Handler of the MSG_WORKLOAD messages
 *  @format : <POLICY> <NUM CLIENTS> <QUERY NUM> <ITERATIONS PER CLIENT> <THINK TIME>
 *  @action : It instanciates a corresponding workload_t  
 */

void parser_t::handle_msg_workload(char* cmd) {

    // Example: OS 1 0 10 4
    
    char* tmp;
    char* s_POLICY = strtok_r(cmd, STD_DELIM, &tmp);
    global_dispatcher_policy_set(s_POLICY);


    char* s_CL = strtok_r(NULL, STD_DELIM, &tmp);
    char* s_Q = strtok_r(NULL, STD_DELIM, &tmp);
    char* s_IT = strtok_r(NULL, STD_DELIM, &tmp);
    char* s_TT = strtok_r(NULL, STD_DELIM, &tmp);

    int i_CL = atoi(s_CL);
    int i_Q = atoi(s_Q);
    int i_IT = atoi(s_IT);
    int i_TT = atoi(s_TT);
  
    if ((i_CL > 0) && (i_IT > 0)) {
      
        /* attach new workload */
        TRACE( TRACE_DEBUG, "Attaching new workload\n");
        
        // create a template client
        client_t * t = new client_t(i_Q, i_TT, i_IT, "CL_TEMPLATE");
 
        // get reference to workload factory
        workload_factory* wf = workload_factory::instance();

        if ( wf->attach_clients(i_CL, *t) == 0 ) {
            TRACE( TRACE_DEBUG, "Clients attached\n");
        }
        else {
            TRACE( TRACE_ALWAYS, "Error while attaching new clients\n");
        }

        // print workload factory info after operation
        wf->print_info();
    }
    else {
        TRACE( TRACE_ALWAYS, "Invalid Parameters for new workload. CL = %d AND IT = %d\n", i_CL, i_IT);
    }  
}



/** @fn     : handle_msg_sql(char*)
 *  @brief  : Handler of the MSG_SQL messages
 *  @format : <SQL>
 *  @action : It instanciates a workload_t consisting of one client and setting his SQL
 */

void parser_t::handle_msg_sql(char* cmd) {

    TRACE( TRACE_DEBUG, "MSG_SQL: %s\n", cmd);
    
    // get SQL text
    char* s_SQL;

    // SQL=SELECT L_ORDERKEY FROM LINEITEM;

    s_SQL = strtok(cmd, EQ_DELIM);
    s_SQL = strtok(NULL, EQ_DELIM);

    char* tmp = strchr(s_SQL, QUEST_CHAR);
    tmp[1] = '\0';

    if ((s_SQL != NULL) && (strlen(s_SQL) > MIN_SQL_TEXT)) {      
        /* attach new workload */

        // create a template client
        client_t t = client_t(0, 0, 1, "CL_SQL_TEMPLATE");
        t.set_sql(s_SQL);
        
        // get reference to workload factory
        workload_factory* wf = workload_factory::instance();

        if ( wf->attach_client(t) == 0 ) {
            TRACE( TRACE_DEBUG, "Client attached\n");
        }
        else {
            TRACE( TRACE_ALWAYS, "Error while attaching new client\n");
        }

        // print workload factory info after operation
        wf->print_info();

    }
    else {
        TRACE( TRACE_DEBUG, "Invalid Parameters for new workload. SQL = %s\n", s_SQL);
    }  
}
 


/** @fn     : handle_msg_workload_info(char*)
 *  @brief  : Handler of the MSG_WORKLOAD_INFO messages
 *  @format : empty
 *  @action : Sends info about all the active workloads.
 */

void parser_t::handle_msg_workload_info(char*) {
    
    // get reference to workload factory
    workload_factory* wf = workload_factory::instance();
    wf->print_wls_info();
}



/** @fn    : print_usage(FILE*)
 *  @brief : Print usage information for QPipe
 *  @param : FILE* out_stream - The stream to print to.
 */

void parser_t::print_usage(FILE* out_stream) {

        fprintf(out_stream, "-----------\n");
        fprintf(out_stream, "QPipe v2.0:\n");
        fprintf(out_stream, "-----------\n");
}



#include "engine/namespace.h" 
