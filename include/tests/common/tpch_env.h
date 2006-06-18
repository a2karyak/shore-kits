
#ifndef _TPCH_ENV_H
#define _TPCH_ENV_H

#include <db_cxx.h>


// environment
extern DbEnv* dbenv;


// tables
extern Db* tpch_customer;
extern Db* tpch_lineitem;
extern Db* tpch_nation;
extern Db* tpch_orders;
extern Db* tpch_part;
extern Db* tpch_partsupp;
extern Db* tpch_region;
extern Db* tpch_supplier;


#endif
