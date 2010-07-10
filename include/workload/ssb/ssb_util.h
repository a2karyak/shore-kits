#ifndef __SSB_UTIL_H
#define __SSB_UTIL_H

#include "workload/ssb/ssb_struct.h"

ENTER_NAMESPACE(ssb);

ssb_l_shipmode str_to_shipmode(char* shipmode);
void shipmode_to_str(char* str, ssb_l_shipmode shipmode);

EXIT_NAMESPACE(ssb);
#endif
