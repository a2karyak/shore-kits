// -*- mode:C++; c-basic-offset:4 -*-

/** @file    : qpipe_parser.cpp
 *  @brief   : Implementation of parser of SQL and QPipe commands
 *  @version : 0.1
 *  @history :
 6/11/2006: Initial version
*/ 

# include "workload/parser.h"



// include me last!!!
#include "engine/namespace.h"


/* CMD constants */
#define STD_DELIM " "
#define MAX_ARGS 4
#define MAX_ARG_SIZE 20
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

#define REC_DELIM "|"
#define QUEST_DELIM ";"
#define QUEST_CHAR ';'
#define EQ_DELIM "="
#define BUF_SIZE 80

#define MIN_SQL_TEXT 10

#define MSG_WORKLOAD       0
#define MSG_SQL            1
#define MSG_WORKLOAD_INFO  2


//////////////////////
// class parser_t



/** @fn     : parse()
 *  @brief  : Parses the passed command and acts accordingly
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
    char* msg_type = strtok_r(cmd, REC_DELIM, &tmpbuf);
    char* msg_cmd = strtok_r(NULL, REC_DELIM, &tmpbuf);
    
    TRACE( TRACE_DEBUG, "TYPE = %s\tCMD = %s\n", msg_type, msg_cmd);

    switch (atoi(msg_type)) {

    case (MSG_WORKLOAD):
        handle_msg_workload(msg_cmd);
        break;

    case (MSG_SQL):
        handle_msg_sql(msg_cmd);
        break;

    case (MSG_WORKLOAD_INFO):
        handle_msg_workload_info(msg_cmd);
        break;

    default:
        TRACE( TRACE_DEBUG, "+ NOT UNDERSTOOD COMMAND: %s\n", msg_cmd);
        return (1);
    }

    free (cmd);
    return (0);
}


/** @fn     : handle_msg_workload(char*)
 *  @brief  : Handler of the MSG_WORKLOAD messages
 *  @format : CL=int;Q=int;IT=int;TT=int
 *  @action : It instanciates a corresponding tpch_workload_t  
 */

void parser_t::handle_msg_workload(char* cmd) {

    char* tmp;
    char* s_CL;
    char* s_Q;
    char* s_IT;
    char* s_TT;

    int i_CL = 0;
    int i_Q = 0;
    int i_IT = 0;
    int i_TT = 0;

    // CL=1;Q=0;IT=10;TT=0;
 
    s_CL = strtok_r(cmd, QUEST_DELIM, &tmp);
    s_Q = strtok_r(NULL, QUEST_DELIM, &tmp);
    s_IT = strtok_r(NULL, QUEST_DELIM, &tmp);
    s_TT = strtok_r(NULL, QUEST_DELIM, &tmp);

    // gets CL
    s_CL = strtok(s_CL, EQ_DELIM);
    s_CL = strtok(NULL, EQ_DELIM);

    // gets Q
    s_Q = strtok(s_Q, EQ_DELIM);
    s_Q = strtok(NULL, EQ_DELIM);

    // gets IT
    s_IT = strtok(s_IT, EQ_DELIM);
    s_IT = strtok(NULL, EQ_DELIM);

    // gets TT
    s_TT = strtok(s_TT, EQ_DELIM);
    s_TT = strtok(NULL, EQ_DELIM);
  
    fprintf(stdout, "WORKLOAD: CL=%d\tQ=%d\tIT=%d\tTT=%d\n", atoi(s_CL), atoi(s_Q), atoi(s_IT), atoi(s_TT));  

    i_CL = atoi(s_CL);
    i_Q = atoi(s_Q);
    i_IT = atoi(s_IT);
    i_TT = atoi(s_TT);

    if ((i_CL > 0) && (i_IT > 0)) {
      
        /* attach new workload */

        // create a template client
        client_t t = client_t(i_Q, i_TT, i_IT, NULL);

        // get reference to workload factory
        workload_factory* wf = workload_factory::instance();

        if ( wf->attach_clients(i_CL, t) == 0 ) {
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
 *  @format : SQL=text
 *  @action : It instanciates a workload_t consisting of one client and setting his SQL
 */

void parser_t::handle_msg_sql(char* cmd) {

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
        client_t t = client_t(0, 0, 1, s_SQL);

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

void parser_t::handle_msg_workload_info(char* ) {

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
