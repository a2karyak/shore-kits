// -*- mode:C++; c-basic-offset:4 -*-

/** @file    : qpipe.cpp
 *  @brief   : QPipe shell related declarations
 *  @version : 0.1
 *  @history :
 6/11/2006: Initial version
*/ 

# ifndef __QPIPE_H
# define __QPIPE_H

/* BerkeleyDB */
#include "db_cxx.h"

/* QPipe Engine */
#include "engine/thread.h"
#include "engine/dispatcher.h"
#include "tests/common/tester_thread.h"
#include "trace.h"
#include "qpipe_panic.h"

/* QPipe Server */
#include "workload.h"
#include "job.h"



/* Configurable values */
#define QPIPE_VERSION "v2.0"
#define QPIPE_PROMPT "(qpipe) "

/* Default configuration values */
#define STD_CONFIG_FILE "qpipe.conf"
#define STD_DATABASE_HOME   "."
#define STD_CONFIG_DATA_DIR "./database"

using namespace qpipe;


using std::string;
using std::ostream;
using std::cout;
using std::cerr;
using std::endl;


/* Start/Stop QPipe environment */
void closeQPipe();
void initQPipe();

/* Read configuration */
void read_config(char* confFile);

/* Open/Close DB tables */
void open_db_tables();
void close_db_tables();

/* Init/Stop stages */
void init_stages();
void stop_stages();

/* Shell function */
static void prompt();
static void execute_cmd(int argc, char* argv[]);

#endif // __QPIPE_H

