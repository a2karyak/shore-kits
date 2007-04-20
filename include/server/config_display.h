/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _CONFIG_DISPLAY_H
#define _CONFIG_DISPLAY_H


class config_display_t {
public:
    virtual void display_config()=0;
};


/* The server is expected to provide a global configuration set. */
config_display_t* server_get_config_display();


#endif
