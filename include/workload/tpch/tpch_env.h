/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _TPCH_ENV_H
#define _TPCH_ENV_H

#include <vector>
#include "core.h"

typedef std::vector<qpipe::page*> page_list;

// tables
extern page_list* tpch_customer;
extern page_list* tpch_lineitem;
extern page_list* tpch_nation;
extern page_list* tpch_orders;
extern page_list* tpch_part;
extern page_list* tpch_partsupp;
extern page_list* tpch_region;
extern page_list* tpch_supplier;


#endif
