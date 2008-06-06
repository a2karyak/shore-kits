/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __UTIL_HISTORY_H
#define __UTIL_HISTORY_H

#include "util/config.h"


#if USE_READLINE

bool history_open();
bool history_close();

#endif


#endif /** __UTIL_HISTORY_H */
