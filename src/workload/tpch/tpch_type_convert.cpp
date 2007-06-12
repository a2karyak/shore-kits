/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpch_type_convert.cpp
 *
 *  @brief Implementation of TPC-H types conversion functions
 */


#include "workload/tpch/tpch_type_convert.h"

#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cstdio>


ENTER_NAMESPACE(tpch);


/** TPC-H type conversions */

tpch_l_shipmode modestr_to_shipmode(char const* tmp) {
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


tpch_o_orderpriority prioritystr_to_orderpriorty(char const* tmp) {

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


/** @fn nnamestr_to_nname
 *  @brief Returns the nation id based on the nation name (string)
 */

tpch_n_name nnamestr_to_nname(char const* tmp) {

    uint i = 0;
    for (i = 0; i < sizeof(nation_names)/sizeof(char*); ++i) {
        if (!strcmp(nation_names[i], tmp))
            return nation_ids[i];
    }

    // should not reach this line
    return UNITED_STATES;
}


EXIT_NAMESPACE(tpch);

