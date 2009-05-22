/* -*- mode:C++; c-basic-offset:4 -*- */


#include "util/history.h"
#include "util/c_str.h"
#include "util/trace.h"



/*
  This whole module is only necessary if we are using readline.
*/
#if USE_READLINE


#ifdef __SUNPRO_CC
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#else
#include <cstdlib>
#include <cstdio>
#include <cstring>
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <readline/history.h>



bool ensure_sclient_directory_exists();
bool try_history_load();
bool history_save();



/**
 * @brief If we are using readline library, we probably want save a
   command history so we don't have to re-type the same commands every
   time we start SCLIENT. history_open() will return true if and only if
   one of the following conditions are met:
   
   the  configuration files exists and
*/
bool history_open() {

    if (!ensure_sclient_directory_exists())
        return false;       

    if (!try_history_load())
        return false;
    
    return true;
}



bool history_close() {
    
    if (!history_save())
        return false;

    return true;
}



bool ensure_sclient_directory_exists() {

    char* home_directory = getenv("HOME");
    if (home_directory == NULL) {
        TRACE(TRACE_ALWAYS,
              "HOME environment variable not set. Cannot use history.\n");
        return false;
    }

    c_str sclient_directory("%s/%s", home_directory, SCLIENT_DIRECTORY_NAME);

    int mkdir_ret = mkdir(sclient_directory.data(), S_IRWXU);
    if ((mkdir_ret != 0) && (errno != EEXIST)) {
        TRACE(TRACE_ALWAYS, "Could not create SCLIENT config directory %s\n",
              sclient_directory.data());
        return false;
    }

    return true;
}



bool try_history_load() {

    char* home_directory = getenv("HOME");
    if (home_directory == NULL) {
        TRACE(TRACE_ALWAYS,
              "HOME environment variable not set. Cannot use history.\n");
        return false;
    }

    c_str sclient_history_file("%s/%s/%s",
                             home_directory,
                             SCLIENT_DIRECTORY_NAME,
                             SCLIENT_HISTORY_FILE);

    int open_ret = open(sclient_history_file.data(), O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
    if (open_ret == -1) {
        TRACE(TRACE_ALWAYS, "Could not open SCLIENT history file %s\n",
              sclient_history_file.data());
        return false;
    }
    close(open_ret);

    if ( read_history(sclient_history_file.data()) ) {
        TRACE(TRACE_ALWAYS, "Could not read SCLIENT history file %s\n",
              sclient_history_file.data());
        return false;
    }

    return true;
}



bool history_save() {

    char* home_directory = getenv("HOME");
    if (home_directory == NULL) {
        TRACE(TRACE_ALWAYS,
              "HOME environment variable not set. Cannot use history.\n");
        return false;
    }

    c_str sclient_history_file("%s/%s/%s",
                             home_directory,
                             SCLIENT_DIRECTORY_NAME,
                             SCLIENT_HISTORY_FILE);

    if ( write_history(sclient_history_file.data()) ) {
        TRACE(TRACE_ALWAYS, "Could not write SCLIENT history file %s\n",
              sclient_history_file.data());
        return false;
    }


    return true;
}



#endif
