
#ifndef _DATASTR_TO_TIMET_H
#define _DATASTR_TO_TIMET_H

#include "tests/common/tpch_struct.h"
#include <sys/time.h>

int datepart(char* str, const time_t *pt);
time_t datestr_to_timet(char* str);
tpch_l_shipmode modestr_to_shipmode(char* tmp);
tpch_o_orderpriority prioritystr_to_orderpriorty(char* tmp);
tpch_n_name nnamestr_to_nname(char* tmp);

#endif


