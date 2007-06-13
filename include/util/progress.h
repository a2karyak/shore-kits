/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file progress.h
 *
 *  @brief Declaration of progress helper functions
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __UTIL_PROGRESS_H
#define __UTIL_PROGRESS_H


#define PROGRESS_INTERVAL 100000


#define MAX_LINE_LENGTH 1024


/** exported helper functions */

void progress_reset(unsigned long* indicator);
void progress_update(unsigned long* indicator);
void progress_done();


/* @note This is not a progress helper function but it is used by
 *       both the tbl_parsers.
 */

void store_string(char* dest, char* src);


#endif

