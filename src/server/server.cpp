// -*- mode:C++; c-basic-offset:4 -*-

/** @file    : qpipe.cpp
 *  @brief   : Basic shell functionality
 *  @version : 0.1
 *  @history :
 6/11/2006: Initial version
*/ 

#include "server.h"
#include "workload/register_job_drivers.h"
#include "workload/register_stage_containers.h"
#include "workload/tpch/tpch_db.h"


using namespace qpipe;

char BUFFER[255];
extern uint32_t trace_current_setting;


/** @fn    : main(int, char* [])
 *  @brief : QPipe server entry point
 */

//int main(int argc, char * argv[]) {
int main(int argc, char * argv[]) {

    fprintf(stdout, "QPipe Execution Engine\n");

    /* initialize stages */
    initQPipe();

    if (argc > 1) {
        /* execute cmd and exit */
        execute_cmd(argc, argv);
    }
    else {
        /* run in interactive mode */
        prompt();
    }
    
    /* closes the server */
    closeQPipe();

    /* exits */
    exit(EXIT_SUCCESS);
    
    /* control never reaches here */
    QPIPE_PANIC();
    return 0; 
}



/** @fn    : prompt()
 *  @brief : Prints the prompt and passes the user input into the parser
 */ 

static void prompt () {

        
    /* Get reference to workload factory parser */
    workload_factory* wf = workload_factory::instance();
    parser_t* parser = wf->get_parser();
    
    while(1) {
    
        printf(QPIPE_PROMPT);

        if(!fgets(BUFFER, sizeof(BUFFER), stdin)) {
            printf("Exiting QPipe\n");
            exit(0);
        }

        if(BUFFER[strlen(BUFFER)-1] == '\n') {
            BUFFER[strlen(BUFFER)-1] = '\0';
        }

        /* run command */
        if ( parser->parse(string(BUFFER))) {
            
            TRACE( TRACE_ALWAYS, "command: '%s' not understood\n", BUFFER);
            parser->print_usage(stderr);
        }
    }
        
    fflush (stdout);
    fflush (stderr);
}




/** @fn    : execute_cmd(int, char * [])
 *  @brief : Executes the passed command
 */

static void execute_cmd(int argc, char* argv [] ) {
    
    if (argc > 2) {
                
        /* Get reference to workload factory parser */
        workload_factory* wf = workload_factory::instance();
        parser_t* parser = wf->get_parser();

        /* set not interactive */
        wf->set_interactive(0);
        
        string one_cmd;
        for (int i=1; i<argc-1; i++) {
            one_cmd += argv[i];
            one_cmd += " ";
        }
        one_cmd += argv[argc-1];

        /* run a single command command */
        if ( parser->parse(one_cmd) ) {
            TRACE( TRACE_ALWAYS, "command: '%s' not understood\n", BUFFER);
            parser->print_usage(stderr);
        }
    }    
}



/** @fn    : initQPipe() 
 *  @brief : Initializes the stages 
 */

void initQPipe( ) {

    /* initialize thread module */
    thread_init();
    
    /* initialize random seed */
    init_srand((uint)pthread_self());

    /* read configuration file */
    read_config(STD_CONFIG_FILE);


    /* open DB tables */
    if ( !db_open() ) {
        TRACE(TRACE_ALWAYS, "db_open() failed\n");
        QPIPE_PANIC();
    }        
    
    /* initialize stages */
    register_stage_containers();

    /* register jobs */
    register_job_drivers();
    
    /* get reference to workload factory - instanciates factory */
    workload_factory* wf = workload_factory::instance();
    wf->print_info();

    // trace_current_setting = TRACE_ALWAYS | TRACE_CPU_BINDING;
    trace_current_setting = TRACE_ALWAYS;
}



/** @fn    : closeQPipe()
 *  @brief : Stops everything and exits
 */

void closeQPipe() {

    /* close DB tables */
    if ( !db_close() )
        TRACE(TRACE_ALWAYS, "db_close() failed\n");
    
    /* @TODO: Make the actuall close DB calls */
    
    fprintf(stdout, "... closing db\n");
    fprintf(stdout, "Thank you for using QPipe\nExiting...\n");
    fflush(stdout);
}




/** @fn    : read_config()
 *  @brief : Opens the configuration file and reads it. On error uses the default (STD_) values
 */

void read_config(char* ) {

    /* @TODO: Should read configuration file */
    
}


/** @fn    : stop_stages()
 *  @brief : Stops all the stages
 */

void stop_stages() {

    /* @TODO: Should stop all the started stages */
    
}
