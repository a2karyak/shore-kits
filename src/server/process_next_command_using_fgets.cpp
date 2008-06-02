
#include "server/process_next_command_using_fgets.h"

#include "util/config.h"
#include "util/chomp.h"

#include "server/print.h"
#include "server/command_set.h"




/**
 * @brief This function reads a command line from the specified
 * stream, chops off the trailing '\n', and invokes process_command()
 * on it. It returns PROCESS_NEXT_QUIT upon reading EOF. It returns
 * the return value of process_command() otherwise.
 *
 * @param in_stream The stream to read from.
 *
 * @return PROCESS_NEXT_QUIT on EOF. The return value of
 * process_command() otherwise.
 */
int process_next_command_using_fgets(FILE* in_stream, bool print_prompt) {

    char command[SERVER_COMMAND_BUFFER_SIZE];

    if (print_prompt)
        TRACE(TRACE_ALWAYS, "%s", QPIPE_PROMPT);
    
    char* fgets_ret = fgets(command, sizeof(command), in_stream);
    if (fgets_ret == NULL)
        // EOF
        return PROCESS_NEXT_QUIT;

    // chomp off trailing '\n', if it exists
    chomp_newline(command);
    chomp_carriage_return(command);
    
    return process_command(command);
}
