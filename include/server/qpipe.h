// -*- mode:C++; c-basic-offset:4 -*-

/** @file    : qpipe.cpp
 *  @brief   : QPipe shell related declarations
 *  @version : 0.1
 *  @history :
 6/11/2006: Initial version
*/ 

# ifndef __QPIPE_H
# define __QPIPE_H

/* Standard */
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <sys/types.h>
#include <errno.h>
#include <iostream>
#include <stddef.h>
#include <stdio.h>
#include <cstdlib>
#include <string>

/* BerkeleyDB */
#include "db_cxx.h"

/* QPipe Engine */
#include "thread.h"
#include "trace/trace.h"
#include "qpipe_panic.h"
#include "dispatcher.h"
#include "tester_thread.h"

/* QPipe Server */
#include "qpipe_wl.h"

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


#endif // __QPIPE_H

