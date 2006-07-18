
#include "engine/util/c_str.h"

const c_str::c_str_data c_str::_seed = { PTHREAD_MUTEX_INITIALIZER, 1, {} };

const c_str c_str::empty_string = "";
