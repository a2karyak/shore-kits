/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util/shell.h"

// class tree_shell : public shell_t
// {
// public:

//     tree_shell(const char* command)
//         : shell_t(command, true)
//     {
//     }

//     virtual ~tree_shell() { }

// };

class kati_allo : public kati_t
{
public:

    kati_allo()
        : kati_t("allou", true)
    { }

    ~kati_allo() { }

    int process_command(const char* cmd);
    int print_usage(const char* cmd) { printf("help-%s", cmd); return(1); }

};


int kati_allo::process_command(const char* cmd) 
{
    printf("%s", cmd); 
    return(0); 
}



int main(int argc, char* argv[]) 
{
    // test of shell
    shell_t* tshell = new shell_t("kati", true);
    tshell->start();
    if (tshell)
        delete (tshell);

    kati_allo ak;
    ak.start();

    return (0);
}



