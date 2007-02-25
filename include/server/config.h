/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _CONFIG_H
#define _CONFIG_H

#define SERVER_COMMAND_BUFFER_SIZE 1024

/* Configurable values */
#define QPIPE_VERSION "v2.0"
#define QPIPE_PROMPT  "(qpipe) "

// Until we get CC to recognize our libreadline install...
#define USE_READLINE 1

/* Default configuration values */
#define QPIPE_DIRECTORY_NAME ".qpipe"
#define QPIPE_HISTORY_FILE   "history"

#define QPIPE_NETWORK_MODE_DEFAULT_LISTEN_PORT 10000


#endif
