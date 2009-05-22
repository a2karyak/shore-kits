/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
      Data Intensive Applications and Systems Labaratory (DIAS)
               Ecole Polytechnique Federale de Lausanne
   
                         All Rights Reserved.
   
   Permission to use, copy, modify and distribute this software and
   its documentation is hereby granted, provided that both the
   copyright notice and this permission notice appear in all copies of
   the software, derivative works or modified versions, and any
   portions thereof, and that both notices appear in supporting
   documentation.
   
   This code is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
   DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
   RESULTING FROM THE USE OF THIS SOFTWARE.
*/

/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file time_util.cpp
 * 
 *  @brief Miscellaneous time-related utilities
 * 
 *  @author Ippokratis Pandis (ipandis)
 */

#include "util/time_util.h"

#include <cassert>

#ifdef __SUNPRO_CC
#include <strings.h>
#include <stdio.h>
#endif 


/** time conversion functions */


/** @fn datepart
 * 
 *  @brief Returns the date part of the time parameter
 */

int datepart(char const* str, const time_t *pt) {
    struct tm tm_struct;

    localtime_r(pt, &tm_struct);

    if(strcmp(str, "yy") == 0) {
        return (tm_struct.tm_year + 1900);
    }
  
    return 0;
}


/** @fn datestr_to_timet
 *
 *  @brief Converts a string to corresponding time_t
 */

time_t datestr_to_timet(char const* str) {
    // str in yyyy-mm-dd format
    tm time_str;
    int count = sscanf(str, "%d-%d-%d", &time_str.tm_year,
                       &time_str.tm_mon, &time_str.tm_mday);
    assert(count == 3);
    time_str.tm_year -= 1900;
    time_str.tm_mon--;
    time_str.tm_hour = 0;
    time_str.tm_min = 0;
    time_str.tm_sec = 0;
    time_str.tm_isdst = -1;

    return mktime(&time_str);
}


/** @fn timet_to_datestr
 *
 *  @brief Converts a time_t object to a string
 */

char* timet_to_datestr(time_t time) {
    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    localtime_r(&time, &tm);
    char* result = new char[strlen("09/09/2009\n")+1];
    sprintf(result, "%04d-%02d-%02d\n", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);
    return result;
}




/** time_t manipulation functions.
 *
 *  @note The functions below use the Unix timezone functions like mktime()
 *         and localtime_r()
 */



/* First day of the reformation, counted from 1 Jan 1 */
#define REFORMATION_DAY 639787	

/* They corrected out 11 days */
#define MISSING_DAYS 11

/* First day of reformation */
#define THURSDAY 4

/* Offset value; 1 Jan 1 was a Saturday */
#define SATURDAY 6


/* @name days_in_month
 * @brief Number of days in a month, using 0 (Jan) to 11 (Dec). For leap years,
 *   add 1 to February (month 1). 
 */

static const int days_in_month[12] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};


/**
 * time_add_day:
 * @time: A time_t value.
 * @days: Number of days to add.
 *
 * Adds a day onto the time, using local time.
 * Note that if clocks go forward due to daylight savings time, there are
 * some non-existent local times, so the hour may be changed to make it a
 * valid time. This also means that it may not be wise to keep calling
 * time_add_day() to step through a certain period - if the hour gets changed
 * to make it valid time, any further calls to time_add_day() will also return
 * this hour, which may not be what you want.
 *
 * Return value: a time_t value containing @time plus the days added.
 */

time_t time_add_day (time_t time, int days) {
    struct tm tm;

    localtime_r(&time, &tm);
    tm.tm_mday += days;
    tm.tm_isdst = -1;
    
    time_t calendar_time = mktime (&tm);

    return calendar_time;
}



/**
 * time_add_week:
 * @time: A time_t value.
 * @weeks: Number of weeks to add.
 *
 * Adds the given number of weeks to a time value.
 *
 * Return value: a time_t value containing @time plus the weeks added.
 */

time_t time_add_week (time_t time, int weeks) {

	return time_add_day (time, weeks * 7);
}



/**
 * time_add_month:
 * @time: A time_t value.
 * @months: Number of months to add.
 *
 * Adds the given number of months to a time value.
 *
 * Return value: a time_t value containing @time plus the months added.
 */

time_t time_add_month (time_t time, int months) {

    struct tm tm;
    localtime_r(&time, &tm);
    tm.tm_mon += months;
    tm.tm_isdst = -1;
    return mktime(&tm);
}



/**
 * time_add_year:
 * @time: A time_t value.
 * @years: Number of years to add.
 *
 * Adds the given number of years to a time value.
 *
 * Return value: a time_t value containing @time plus the years added.
 */

time_t time_add_year (time_t time, int years) {

    struct tm tm;
    localtime_r(&time, &tm);
    tm.tm_year += years;
    tm.tm_isdst = -1;
    return mktime(&tm);
}



/**
 * time_day_begin:
 * @t: A time_t value.
 *
 * Returns the start of the day, according to the local time.
 *
 * Return value: the time corresponding to the beginning of the day.
 */

time_t time_day_begin (time_t t) {

  struct tm tm;
  
  localtime_r(&t, &tm);
  tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
  tm.tm_isdst = -1;
  
  return mktime (&tm);
}



/**
 * time_day_end:
 * @t: A time_t value.
 *
 * Returns the end of the day, according to the local time.
 *
 * Return value: the time corresponding to the end of the day.
 */

time_t time_day_end (time_t t) {

  struct tm tm;
  
  localtime_r(&t, &tm);
  tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
  tm.tm_mday++;
  tm.tm_isdst = -1;
  
  return mktime (&tm);
}


