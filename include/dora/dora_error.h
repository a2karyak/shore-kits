/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_error.h
 *
 *  @brief:  Enumuration of DORA-related errors
 *
 *  @author: Ippokratis Pandis, Oct 2008
 *
 */

#ifndef __DORA_ERROR_H
#define __DORA_ERROR_H

#include "util/namespace.h"


ENTER_NAMESPACE(dora);

/* error codes returned from DORA */

/** note: All DORA-related errors start with 0x82.. */

enum {
    de_GEN_WORKER              = 0x820001,
    de_GEN_PRIMARY_WORKER      = 0x820002,
    de_GEN_STANDBY_POOL        = 0x820003,
    
    de_WRONG_ACTION            = 0x820004,
    de_WRONG_PARTITION         = 0x820005,
    de_WRONG_WORKER            = 0x820006,
  
    de_INCOMPATIBLE_LOCKS      = 0x820007
};



EXIT_NAMESPACE(dora);

#endif /* __DORA_ERROR_H */
