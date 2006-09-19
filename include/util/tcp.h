
/** @file tcp.h
 *
 *  @brief Exports simple functions to set up TCP client and server
 *  connections.
 *
 *  @author Naju Mancheril (ngm)
 *
 *  @bug See tcp.c.
 */
#ifndef _TCP_H
#define _TCP_H




/* exported functions */

int open_listenfd(int port);
int open_clientfd(const char* hostname, int port);




#endif
