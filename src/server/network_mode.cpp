/* -*- mode:C++; c-basic-offset:4 -*- */

#include "server/network_mode.h"
#include "server/process_next_command_using_fgets.h"
#include "server/command_set.h"
#include "util/tcp.h"
#include "util/trace.h"
#include "util/compat.h"

#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>  /* for socket */
#include <sys/socket.h>
#include <netdb.h>      /* for clientaddr structure */



#include <stdio.h>
#ifndef __GCC
using namespace std;
#endif


/**
 * @brief Handle a client connection.
 *
 * handle_client is responsible for closing connfd when it is done
 * with it.
 */
static int handle_client(int connfd) {
    
    FILE* conn_stream = fdopen(connfd, "r");
    if (conn_stream == NULL) {
        TRACE(TRACE_ALWAYS, "fdopen() failed on connection descriptor %d\n",
              connfd);
        close(connfd);
        return false;
    }


    /* Network mode cares about QUIT versus SHUTDOWN, but this
       function only distinguishes between PROCESS_NEXT_CONTINUE and
       everything else. Everything else results in us closing the
       connection and passing the value through to our caller. */
    int state = PROCESS_NEXT_CONTINUE;
    while (state == PROCESS_NEXT_CONTINUE) {
        state = process_next_command_using_fgets(conn_stream, false);
    }

    fclose(conn_stream);
    return state;
}



void network_mode(int listen_port) {

    /* open listening socket */
    int listenfd = open_listenfd(listen_port);
    if (listenfd == -1) {
        TRACE(TRACE_ALWAYS, "Could not open listen port %d: %s\n",
              listen_port,
              strerror(errno));
        return;
    }


    /* wait for clients */
    /* We will keep running until we get back a PROCESS_NEXT_SHUTDOWN
       from handle_client. */
    int state = PROCESS_NEXT_CONTINUE;
    while (state != PROCESS_NEXT_SHUTDOWN) {

        struct sockaddr_in clientaddr;
        int clientlen = sizeof(clientaddr);
        int connfd;
        
        /* accept new client */
        connfd = accept(listenfd, (struct sockaddr*)&clientaddr, (socklen_t*)&clientlen);
        if ( connfd < 0 ) {
            TRACE(TRACE_ALWAYS, "Got error accepting new client. Listen thread exiting.\n");
            return;
        }

        /* To avoid messy synchronization issues, we will avoid
           creating a new thread. We will just execute the requests
           from the new client in the context of the root thread. */

        TRACE(TRACE_NETWORK, "Opened client connection\n");
        
        state = handle_client(connfd);

        TRACE(TRACE_NETWORK, "Closed client connection\n");
    }
}
