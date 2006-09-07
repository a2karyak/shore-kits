/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/tpch/tpch_type_convert.h"
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <time.h>



int datepart(char* str, const time_t *pt) {
    tm tm_struct;
    tm_struct = *(localtime(pt));
    if(strcmp(str, "yy") == 0) {
        return (tm_struct.tm_year + 1900);
    }
  
    return 0;
}


/** @fn    : datestr_to_timet(char*)
 *  @brief : Converts a string to corresponding time_t
 */

time_t datestr_to_timet(char* str) {
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

char* timet_to_datestr(time_t time) {
    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    localtime_r(&time, &tm);
    char* result;
    asprintf(&result, "%04d-%02d-%02d\n", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);
    return result;
}

tpch_l_shipmode modestr_to_shipmode(char* tmp) {
    if (!strcmp(tmp, "REG AIR"))
        return REG_AIR;
    else if (!strcmp(tmp, "AIR"))
        return AIR;
    else if (!strcmp(tmp, "RAIL"))
        return RAIL;
    else if (!strcmp(tmp, "TRUCK"))
        return TRUCK;
    else if (!strcmp(tmp, "MAIL"))
        return MAIL;
    else if (!strcmp(tmp, "FOB"))
        return FOB;
    else // (!strcmp(tmp, "SHIP"))
        return SHIP;
}


tpch_o_orderpriority prioritystr_to_orderpriorty(char* tmp) {

    if (!strcmp(tmp, "1-URGENT"))
        return URGENT_1;
    else if (!strcmp(tmp, "2-HIGH"))
        return HIGH_2;
    else if (!strcmp(tmp, "3-MEDIUM"))
        return MEDIUM_3;
    else if (!strcmp(tmp, "4-NOT SPECIFIED"))
        return NOT_SPECIFIED_4;
    else // if (!strcmp(tmp, "5-LOW"))
        return LOW_5;
}


char* nation_names[] = {
    "ALGERIA",
    "ARGENTINA",
    "BRAZIL",
    "CANADA",
    "EGYPT",
    "ETHIOPIA",
    "FRANCE",
    "GERMANY",
    "INDIA",
    "INDONESIA",
    "IRAN",
    "IRAQ",
    "JAPAN",
    "JORDAN",
    "KENYA",
    "MOROCCO",
    "MOZAMBIQUE",
    "PERU",
    "CHINA",
    "ROMANIA",
    "SAUDI ARABIA",
    "VIETNAM",
    "RUSSIA",
    "UNITED KINGDOM",
    "UNITED STATES"
};

tpch_n_name nation_ids[] = {
    ALGERIA,
    ARGENTINA,
    BRAZIL,
    CANADA,
    EGYPT,
    ETHIOPIA,
    FRANCE,
    GERMANY,
    INDIA,
    INDONESIA,
    IRAN,
    IRAQ,
    JAPAN,
    JORDAN,
    KENYA,
    MOROCCO,
    MOZAMBIQUE,
    PERU,
    CHINA,
    ROMANIA,
    SAUDI_ARABIA,
    VIETNAM,
    RUSSIA,
    UNITED_KINGDOM,
    UNITED_STATES
};


/**
 * nnamestr_to_nname: Returns the nation id based on the nation name (string)
 */

tpch_n_name nnamestr_to_nname(char* tmp) {

    uint i = 0;
    for (i = 0; i < sizeof(nation_names)/sizeof(char*); ++i) {
        if (!strcmp(nation_names[i], tmp))
            return nation_ids[i];
    }

    // should not reach this line
    return UNITED_STATES;
}
