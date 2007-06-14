/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __COMMAND_SET_H
#define __COMMAND_SET_H

void register_command_handlers(const int environment);


#define PROCESS_NEXT_CONTINUE 1
#define PROCESS_NEXT_QUIT     2
#define PROCESS_NEXT_SHUTDOWN 3

int process_command(const char* command);

void shutdown_command_handlers(void);


#endif
