/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util/stagedtrx_shell.h"

class tree_shell : public stagedtrx_shell_t
{
public:

    tree_shell()
        : stagedtrx_shell_t()
    {
    }

    ~tree_shell() { }

    int process_command(const char* command) {
        printf(".");
        return (0);
    }

    void print_usage(int argc, char* argv[]) {
        printf("_");
    }         

};


int main(int argc, char* argv[]) 
{
    // test of shell
    tree_shell tshell;
    tshell.start();
    return (0);
}



