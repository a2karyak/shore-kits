/* Miscellaneous time-related utilities
 *
 * Copyright (C) 2000, 2001 Ximian, Inc.
 *
 * Authors: Federico Mena <federico@ximian.com>
 *          Miguel de Icaza <miguel@ximian.com>
 *          Damon Chaplin <damon@ximian.com>
 *
 * History:
 * 3/3/2006: Modified and integrated to QPipe by Ippokratis Pandis 
 */

#include "util/time.h"

#include <cassert>


#define REFORMATION_DAY 639787	/* First day of the reformation, counted from 1 Jan 1 */
#define MISSING_DAYS 11		/* They corrected out 11 days */
#define THURSDAY 4		/* First day of reformation */
#define SATURDAY 6		/* Offset value; 1 Jan 1 was a Saturday */


/* Number of days in a month, using 0 (Jan) to 11 (Dec). For leap years,
   add 1 to February (month 1). */
static const int days_in_month[12] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};



/**************************************************************************
 * time_t manipulation functions.
 *
 * NOTE: these use the Unix timezone functions like mktime() and localtime()
 * and so should not be used in Evolution. New Evolution code should use
 * icaltimetype values rather than time_t values wherever possible.
 **************************************************************************/

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
time_t
time_add_day (time_t time, int days)
{
    struct tm tm;

	localtime_r (&time, &tm);
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
time_t
time_add_week (time_t time, int weeks)
{
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
time_t
time_add_month (time_t time, int months)
{
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
time_t
time_add_year (time_t time, int years)
{
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
time_t
time_day_begin (time_t t)
{
	struct tm tm;

	localtime_r (&t, &tm);
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
time_t
time_day_end (time_t t)
{
	struct tm tm;

	localtime_r (&t, &tm);
	tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
	tm.tm_mday++;
	tm.tm_isdst = -1;

	return mktime (&tm);
}


