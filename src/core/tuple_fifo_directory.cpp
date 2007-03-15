/** -*- mode:C++; c-basic-offset:4 -*- */

#include "util/thread.h"
#include "util/sync.h"
#include "util/fileops.h"
#include "core/tuple_fifo_directory.h"



ENTER_NAMESPACE(qpipe);


pthread_mutex_t 
tuple_fifo_directory_t::_dir_mutex = PTHREAD_MUTEX_INITIALIZER;

tuple_fifo_directory_t::dir_state_t
tuple_fifo_directory_t::_dir_state = TUPLE_FIFO_DIRECTORY_FIRST;

c_str
tuple_fifo_directory_t::_dir_path = c_str("temp");



const c_str& tuple_fifo_directory_t::dir_path() {
    /* TODO Make this a configuration variable */
    return _dir_path;
}



void tuple_fifo_directory_t::open_once() {

    critical_section_t cs(_dir_mutex);
    if (_dir_state != TUPLE_FIFO_DIRECTORY_FIRST)
        return;

    const c_str& path = dir_path();
    if (fileops_check_file_directory(path.data()))
        THROW2(TupleFifoDirectoryException,
               "Tuple fifo directory %s does not exist\n",
               path.data());

    if (fileops_check_directory_accessible(path.data()))
        THROW2(TupleFifoDirectoryException,
               "Tuple fifo directory %s not writeable\n",
               path.data());

    _dir_state = TUPLE_FIFO_DIRECTORY_OPEN;
}



void tuple_fifo_directory_t::close() {

    critical_section_t cs(_dir_mutex);
    if (_dir_state != TUPLE_FIFO_DIRECTORY_OPEN)
        return;

    _dir_state = TUPLE_FIFO_DIRECTORY_CLOSED;
}



EXIT_NAMESPACE(qpipe);
