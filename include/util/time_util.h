/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file time_util.h
 * 
 *  @brief Miscellaneous time-related utilities
 * 
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TIME_UTIL_H
#define __TIME_UTIL_H

#include <time.h>

#include <cstdio>
#include <cstring>
#include <ctype.h>
#include <cstdlib>


/** time conversion functions */

int datepart(char const* str, const time_t *pt);
time_t datestr_to_timet(char const* str);
char* timet_to_datestr(time_t time);



/** time_t manipulation functions
 *
 *  @note These function use the Unix timezone functions
 *        like mktime() and localtime()
 */

/* Add or subtract a number of days, weeks or months */
time_t time_add_day(time_t time, int days);
time_t time_add_week(time_t time, int weeks);
time_t time_add_month(time_t time, int months);
time_t time_add_year(time_t time, int years);

/* Return the beginning or end of the day */
time_t time_day_begin(time_t t);
time_t time_day_end(time_t t);



#endif
