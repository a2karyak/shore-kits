// -*- mode:C++; c-basic-offset:4 -*-

/** @file    : qpipe.cpp
 *  @brief   : Basic shell functionality
 *  @version : 0.1
 *  @history :
 6/11/2006: Initial version
*/ 

#include "server.h"

using namespace qpipe;

char BUFFER[255];


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
    open_db_tables();
    
    /* initialize stages */
    init_stages();

    /* get reference to workload factory - instanciates factory */
    workload_factory* wf = workload_factory::instance();
    wf->print_info();
}



/** @fn    : closeQPipe()
 *  @brief : Stops everything and exits
 */

void closeQPipe() {

    /* close DB tables */
    close_db_tables();
    
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



/** @fn    : open_db_tables()
 *  @brief : Opens DB tables
 */

void open_db_tables() {

    /* @TODO: Should contact Catalog */
    
}



/** @fn    : close_db_tables()
 *  @brief : Closes DB tables
 */

void close_db_tables() {

    /* @TODO: Should contact Catalog */
    
}



/** @fn    : init_stages()
 *  @brief : Initializes/Starts all the stages
 */

void init_stages() {

    /* @TODO: Should start only the configured stages */
    
}



/** @fn    : stop_stages()
 *  @brief : Stops all the stages
 */

void stop_stages() {

    /* @TODO: Should stop all the started stages */
    
}
