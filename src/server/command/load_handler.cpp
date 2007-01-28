#include <ltdl.h>
#include "server/command/load_handler.h"
#include "server/command/tpch_handler.h"

using namespace workload;

typedef scheduler::policy_t* (*policy_factory)();
typedef workload::driver_t* (*driver_factory)(const c_str& description,
                                               driver_directory_t* directory);
typedef command_handler_t* (*command_factory)();

extern void add_command(const char*  command_tag, command_handler_t* handler);
extern tpch_handler_t* tpch_handler;

void load_handler_t::init() {
    assert(lt_dlinit() == 0);
}

void load_handler_t::shutdown() {
    lt_dlexit();
}

void load_handler_t::handle_command(char const* command) {
    array_guard_t<char> copy = new char[strlen(command)+1];
    strcpy(copy, command);

    char* saved;

    // skip the command name
    strtok_r(copy, " ", &saved);

    
    // open the shared library
    char* soname = strtok_r(NULL, " ", &saved);
    if(soname == NULL) {
        printf("No shared library named!\n");
        return;
    }
    
    lt_dlhandle handle = lt_dlopenext(soname);
    if(!handle) {
        printf("Unable to open shared library '%s'\n", soname);
        printf("Error was: %s\n", lt_dlerror());
        return;
    }
    
    char* subcommand = strtok_r(NULL, " ", &saved);
    if(subcommand == NULL) {
        printf("No subcommand given!\n");
        return;
    }
    
    if(!strcasecmp(subcommand, "policy")) {
        // load the requested policies
        char* policy;
        while((policy = strtok_r(NULL, " ", &saved))) {
            policy_factory f = (policy_factory) lt_dlsym(handle, policy);
            if(!f) {
                printf("Unable to find factory for policy '%s'\n", policy);
                printf("Error was: %s\n", lt_dlerror());
                continue;
            }

            printf("Registering policy '%s'\n", policy);
            tpch_handler->add_scheduler_policy(policy, f());
        }
    }
    else if(!strcasecmp(subcommand, "driver")) {
        // load the requested drivers
        char* driver;
        while((driver = strtok_r(NULL, " ", &saved))) {
            driver_factory f = (driver_factory) lt_dlsym(handle, driver);
            if(!f) {
                printf("Unable to find factory for driver '%s'\n", driver);
                printf("Error was: %s\n", lt_dlerror());
                continue;
            }

            printf("Registering driver '%s'\n", driver);
            tpch_handler->add_driver(driver, f(driver, tpch_handler));
        }
    }
    else if(!strcasecmp(subcommand, "command")) {
        // load the requested commands
        char* cmd;
        while((cmd = strtok_r(NULL, " ", &saved))) {
            command_factory f = (command_factory) lt_dlsym(handle, cmd);
            if(!f) {
                printf("Unable to find factory for command '%s'\n", cmd);
                printf("Error was: %s\n", lt_dlerror());
                continue;
            }

            printf("Registering command '%s'\n", cmd);
            add_command(cmd, f());
        }
    }
    else {
        printf("Unrecognized load subcommand: %s\n", subcommand);
    }
}
