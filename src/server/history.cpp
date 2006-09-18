/* -*- mode:C++; c-basic-offset:4 -*- */


#include "server/history.h"
#include "server/config.h"
#include "util/c_str.h"
#include "util/trace.h"



/*
  This whole module is only necessary if we are using readline.
*/
#if USE_READLINE

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>



bool ensure_qpipe_directory_exists();



/**
 * @brief If we are using readline library, we probably want save a
   command history so we don't have to re-type the same commands every
   time we start QPIPE. history_open() will return true if and only if
   one of the following conditions are met:
   
   the  configuration files exists and
*/
bool history_open() {

    if (!ensure_qpipe_directory_exists())
        return false;       

    return false;
}


bool history_close() {
    return true;
}




bool ensure_qpipe_directory_exists() {

    char* home_directory = getenv("HOME");
    if (home_directory == NULL) {
        TRACE(TRACE_ALWAYS,
              "HOME environment variable not set. Cannot use history.\n");
        return false;
    }

    c_str qpipe_directory("%s/%s", home_directory, QPIPE_DIRECTORY_NAME);

    int mkdir_ret = mkdir(qpipe_directory.data(), S_IRWXU);
    if ((mkdir_ret != 0) && (errno != EEXIST)) {
        TRACE(TRACE_ALWAYS, "Could not create QPIPE config directory %s\n",
              qpipe_directory.data());
        return false;
    }

    return true;
}


#if 0
/**
 * @brief Check whether the file specified is writeable.
 * @param abs_path The path of the file.
 * 
 * @return 1 If program file is writeable. 0 otherwise.
 */
static int is_writable_file(char* abs_path)
{
    // for simplicity, we will just try and create the file here

  int fd = open(abs_path, O_WRONLY|O_CREAT|O_TRUNC);
  if ( fd == -1 )
  {
    return 0;
  }

  // if we succeed, we will remove the file here as well
  close(fd);
  if ( unlink(abs_path) == -1 )
  {
    dbprintf("Got error removing file: %s\n", strerror(errno));
    return 0;
  }
  return 1;
}
#endif


#endif
