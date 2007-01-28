
#ifndef _PROCESS_NEXT_COMMAND_USING_FGETS_H
#define _PROCESS_NEXT_COMMAND_USING_FGETS_H

#include <cstdio>
#ifndef __GCC
using std::FILE;
#endif

int process_next_command_using_fgets(FILE* in_stream, bool print_prompt);

#endif
