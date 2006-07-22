/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _COMMAND_SET_H
#define _COMMAND_SET_H


void register_command_handlers(void);

int process_command(const char* command);

void shutdown_command_handlers(void);


#endif
