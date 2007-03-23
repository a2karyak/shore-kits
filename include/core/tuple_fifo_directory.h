/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef __TUPLE_FIFO_DIRECTORY_H
#define __TUPLE_FIFO_DIRECTORY_H

#include "util.h"


ENTER_NAMESPACE(qpipe);


DEFINE_EXCEPTION(TupleFifoDirectoryException);


class tuple_fifo_directory_t {

private:
    
    enum dir_state_t {
        TUPLE_FIFO_DIRECTORY_FIRST,
        TUPLE_FIFO_DIRECTORY_OPEN,
        TUPLE_FIFO_DIRECTORY_CLOSED
    };

    static c_str _dir_path;

    static pthread_mutex_t _dir_mutex;
    
    static dir_state_t _dir_state;
    
public:

    static const c_str& dir_path();
    static void open_once();
    static void close();
    static c_str generate_filepath(int id);
    
private:

    static bool filename_filter(const char* path);
    static void clean_dir();

};


EXIT_NAMESPACE(qpipe);


#endif
