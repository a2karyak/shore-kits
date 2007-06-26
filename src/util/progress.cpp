/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file progress.cpp
 *
 *  @brief Implementation of progress helper functions
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "util/progress.h"

#include <cstdio>
#include <cstring>


/* definitions of exported helper functions */


/** @fn progress_reset
 *  @brief Set the indicator to 0
 */

void progress_reset(unsigned long* indicator) {
    *indicator = 0;
}



/** @fn progress_update
 *  @brief Inceases progress by 1, if PROGRESS_INTERVAL outputs a dot
 */

void progress_update(unsigned long* indicator) {
  
    if ( (*indicator++ % PROGRESS_INTERVAL) == 0 ) {
        printf(".");
        fflush(stdout);
        *indicator = 1; // prevent overflow
    }
}



/** @fn progress_done
 *  @brief Outputs a done message
 */

void progress_done(const char* tablename) {
    
    printf("\nDone loading (%s)...\n", tablename);
    fflush(stdout);
}

