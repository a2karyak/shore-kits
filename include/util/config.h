/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __UTIL_SERVER_CONFIG_H
#define __UTIL_SERVER_CONFIG_H

#define SERVER_COMMAND_BUFFER_SIZE 1024

/* Configurable values */
#define SCLIENT_VERSION "v2.1"
#define SCLIENT_PROMPT  "(sparta) "

// Until we get CC to recognize our libreadline install...
#define USE_READLINE 1

// Default configuration values
#define SCLIENT_DIRECTORY_NAME ".shoremtclient"
#define SCLIENT_HISTORY_FILE   "history"

#define SCLIENT_NETWORK_MODE_DEFAULT_LISTEN_PORT 10000


#endif /** __UTIL_SERVER_CONFIG_H */
