
#ifndef _DATASTR_TO_TIMET_H
#define _DATASTR_TO_TIMET_H

#include "workload/tpch/tpch_struct.h"
#include <sys/time.h>

int datepart(char const* str, const time_t *pt);
time_t datestr_to_timet(char const* str);
char* timet_to_datestr(time_t time);
tpch_l_shipmode modestr_to_shipmode(char const* tmp);
tpch_o_orderpriority prioritystr_to_orderpriorty(char const* tmp);
tpch_n_name nnamestr_to_nname(char const* tmp);

#endif


