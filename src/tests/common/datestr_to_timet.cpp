
#include "tests/common/datestr_to_timet.h"
#include <cstdlib>
#include <cstring>
#include <time.h>



/** @fn    : datestr_to_timet(char*)
 *  @brief : Converts a string to corresponding time_t
 */

time_t datestr_to_timet(char* str) {
    char buf[100];
    strcpy(buf, str);

    // str in yyyy-mm-dd format
    char* year = buf;
    char* month = buf + 5;
    // char* day = buf + 8;

    buf[4] = '\0';
    buf[7] = '\0';

    tm time_str;
    time_str.tm_year = atoi(year) - 1900;
    time_str.tm_mon = atoi(month) - 1;
    time_str.tm_mday = 4;
    time_str.tm_hour = 0;
    time_str.tm_min = 0;
    time_str.tm_sec = 1;
    time_str.tm_isdst = -1;

    return mktime(&time_str);
}
