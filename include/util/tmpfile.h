/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __TMPFILE_H
#define __TMPFILE_H

#include <cstdio>
#include "util/c_str.h"


FILE* create_tmp_file(c_str &name, const c_str &prefix);



#endif
