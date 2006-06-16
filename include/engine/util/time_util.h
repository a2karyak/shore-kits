/* Miscellaneous time-related utilities
 *
 * Copyright (C) 1998 The Free Software Foundation
 * Copyright (C) 2000 Ximian, Inc.
 *
 * Authors: Federico Mena <federico@ximian.com>
 *          Miguel de Icaza <miguel@ximian.com>
 *          Damon Chaplin <damon@ximian.com>
 */

# ifndef TIME_UTIL_H
# define TIME_UTIL_H


# include <time.h>

# include <string.h>
# include <ctype.h>
# include <stdlib.h>


/**************************************************************************
 * time_t manipulation functions.
 *
 * NOTE: these use the Unix timezone functions like mktime() and localtime()
 **************************************************************************/

/* Add or subtract a number of days, weeks or months. */
time_t	time_add_day		(time_t time, int days);
time_t	time_add_week		(time_t time, int weeks);

/* Returns the beginning or end of the day. */
time_t	time_day_begin		(time_t t);
time_t	time_day_end		(time_t t);


# endif // TIME_UTIL_H
