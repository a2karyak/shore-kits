/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _HISTORY_H
#define _HISTORY_H

#include "server/config.h"


#if USE_READLINE

bool history_open();
bool history_close();

#endif


#endif
