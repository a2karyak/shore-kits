/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tpcc_schema.h
 *
 *  @brief:  Declaration of the TPC-C tables
 *
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#ifndef __SHORE_TPCC_SCHEMA_H
#define __SHORE_TPCC_SCHEMA_H

#include <math.h>

#include "sm_vas.h"
#include "util.h"

#include "stages/tpcc/common/tpcc_const.h"
#include "stages/tpcc/common/tpcc_tbl_parsers.h"
#include "sm/shore/shore_table_man.h"
#include "stages/tpcc/shore/shore_tpcc_random.h"


using namespace shore;


ENTER_NAMESPACE(tpcc);



/*********************************************************************
 *
 * TPC-C SCHEMA
 * 
 * This file contains the classes for tables in tpcc benchmark.
 * A class derived from tpcc_table_t (which inherits from table_desc_t) 
 * is created for each table in the databases.
 *
 *********************************************************************/


/*
 * indices created on the tables are:
 *
 * 1. unique index on order_line(ol_w_id,ol_d_id,ol_o_id,ol_number)
 * 2. unique index on stock(s_w_id,s_i_id)
 * 3. unique index on customer(c_w_id,c_d_id,c_id)
 * 4. index on customer(c_w_id,c_d_id,c_last,c_first,c_id)
 * 5. unique index on order(o_w_id,o_d_id,o_id)
 * 6. unique index on order(o_w_id,o_d_id,o_c_id,o_id) desc
 * 7. unique index on item(i_id)
 * 8. unique index on new_order(no_w_id,no_d_id,no_o_id)
 * 9. unique index on district(d_id,d_w_id)
 * 10. unique index on warehouse(w_id)
 */


/*
 * Define CREATE_NOLOCKING_INDEXES in order to create indexes that do 
 * not use locking -- used by stagedtrx
 */
#undef  CREATE_NOLOCKING_INDEXES
//#define CREATE_NOLOCKING_INDEXES

#undef  CREATE_CUST_NOLOCK_INDEXES
#define CREATE_CUST_NOLOCK_INDEXES




/*  --------------------------------------------------------------
 *
 *  @class: tpcc_table_desc_t
 *
 *  @brief: Base class for all the TPC-C tables
 *
 *  @note:  It simply extends the table_desc_t class
 *
 *  --------------------------------------------------------------- */

class tpcc_table_t : public table_desc_t {
public:

    tpcc_table_t(const char* name, int fieldcnt)
        : table_desc_t(name, fieldcnt)
    {
    }

    virtual ~tpcc_table_t() { }

}; // EOF: tpcc_table_t


typedef std::list<tpcc_table_t*> tpcc_table_list_t;
typedef std::list<tpcc_table_t*>::iterator tpcc_table_list_iter;


/* -------------------------------------------------- */
/* --- All the tables used in the TPC-C benchmark --- */
/* -------------------------------------------------- */


class warehouse_t : public tpcc_table_t 
{
public:
    warehouse_t() : tpcc_table_t("WAREHOUSE", TPCC_WAREHOUSE_FCOUNT) {
        /* table schema */
        _desc[0].setup(SQL_INT,   "W_ID");
        _desc[1].setup(SQL_CHAR,  "W_NAME", 10);
        _desc[2].setup(SQL_CHAR,  "W_STREET1", 20);
        _desc[3].setup(SQL_CHAR,  "W_STREET2", 20);
        _desc[4].setup(SQL_CHAR,  "W_CITY", 20);   
        _desc[5].setup(SQL_CHAR,  "W_STATE", 2);
        _desc[6].setup(SQL_CHAR,  "W_ZIP", 9);
        _desc[7].setup(SQL_FLOAT, "W_TAX");   
        _desc[8].setup(SQL_FLOAT, "W_YTD");  /* DECIMAL(12,2) */
        
        /* create unique index w_index on (w_id) */
        int  keys[1] = { 0 };
        create_primary_idx("W_INDEX", keys, 1);

#ifdef CREATE_NOLOCKING_INDEXES
        // last param (nolock) is set to true
        create_index("W_INDEX_NOLOCK", keys, 1, true, false, true);
#endif
    }

    bool read_tuple_from_line(table_row_t& tuple, char* buf);

}; // EOF: warehouse_t



class district_t : public tpcc_table_t 
{
public:
    district_t() : tpcc_table_t("DISTRICT", TPCC_DISTRICT_FCOUNT) {
        /* table schema */
        _desc[0].setup(SQL_INT,   "D_ID");     
        _desc[1].setup(SQL_INT,   "D_W_ID");   
        _desc[2].setup(SQL_CHAR,  "D_NAME", 10);    /* VARCHAR(10) */ /* old: CHAR */
        _desc[3].setup(SQL_CHAR,  "D_STREET1", 20);
        _desc[4].setup(SQL_CHAR,  "D_STREET2", 20);
        _desc[5].setup(SQL_CHAR,  "D_CITY", 20);
        _desc[6].setup(SQL_CHAR,  "D_STATE", 2); 
        _desc[7].setup(SQL_CHAR,  "D_ZIP", 9);   
        _desc[8].setup(SQL_FLOAT, "D_TAX");    
        _desc[9].setup(SQL_FLOAT, "D_YTD");         /* DECIMAL(12,2) */
        _desc[10].setup(SQL_INT,  "D_NEXT_O_ID");

        /* create unique index d_index on (d_id, w_id) */
        int keys[2] = { 0, 1 };
        create_primary_idx("D_INDEX", keys, 2);

#ifdef CREATE_NOLOCKING_INDEXES
        // last param (nolock) is set to true
        create_index("D_INDEX_NOLOCK", keys, 2, true, false, true);
#endif
    }

    bool read_tuple_from_line(table_row_t& tuple, char* buf);

}; // EOF: district_t



class customer_t : public tpcc_table_t 
{
public:
    customer_t() : tpcc_table_t("CUSTOMER", TPCC_CUSTOMER_FCOUNT) {
        /* table schema */
        _desc[0].setup(SQL_INT,    "C_ID");
        _desc[1].setup(SQL_INT,    "C_D_ID");       
        _desc[2].setup(SQL_INT,    "C_W_ID");       
        _desc[3].setup(SQL_CHAR,   "C_FIRST", 16);  
        _desc[4].setup(SQL_CHAR,   "C_MIDDLE", 2);  
        _desc[5].setup(SQL_CHAR,   "C_LAST", 16);   
        _desc[6].setup(SQL_CHAR,   "C_STREET1", 20);
        _desc[7].setup(SQL_CHAR,   "C_STREET2", 20);
        _desc[8].setup(SQL_CHAR,   "C_CITY", 20);   
        _desc[9].setup(SQL_CHAR,   "C_STATE", 2);   
        _desc[10].setup(SQL_CHAR,  "C_ZIP", 9);     
        _desc[11].setup(SQL_CHAR,  "C_PHONE", 16);  
        _desc[12].setup(SQL_FLOAT, "C_SINCE");           /* old: TIME */
        _desc[13].setup(SQL_CHAR,  "C_CREDIT", 2);  
        _desc[14].setup(SQL_FLOAT, "C_CREDIT_LIM");      /* DECIMAL(12,2) */
        _desc[15].setup(SQL_FLOAT, "C_DISCOUNT");
        _desc[16].setup(SQL_FLOAT, "C_BALANCE");         /* DECIMAL(12,2) */
        _desc[17].setup(SQL_FLOAT, "C_YTD_PAYMENT");     /* DECIMAL(12,2) */
        _desc[18].setup(SQL_FLOAT, "C_LAST_PAYMENT");    /* !! new !! */ /* - DECIMAL(12,2) */
        _desc[19].setup(SQL_INT,   "C_PAYMENT_CNT");     /* used to be [18] */
        //        _desc[19].setup(SQL_SMALLINT, "C_DELIVERY_CNT");
        _desc[20].setup(SQL_CHAR,  "C_DATA_1", 250);
        _desc[21].setup(SQL_CHAR,  "C_DATA_2", 250);     /* !! new !! */

        /* create unique index c_index on (w_id, d_id, c_id) */
        int keys1[3] = {2, 1, 0 };
        create_primary_idx("C_INDEX", keys1, 3);

        /* create index c_name_index on (w_id, d_id, last, first, id) */
        int keys2[5] = {2, 1, 5, 3, 0};
        create_index("C_NAME_INDEX", keys2, 5, false);


#ifdef CREATE_NOLOCKING_INDEXES
        // last param (nolock) is set to true
        create_index("C_INDEX_NOLOCK", keys1, 3, true, false, true);
        create_index("C_NAME_INDEX_NOLOCK", keys2, 5, false, false, true);
#else
#ifdef CREATE_CUST_NOLOCK_INDEXES
        // last param (nolock) is set to true
        create_index("C_INDEX_NOLOCK", keys1, 3, true, false, true);
#endif
#endif
    }


    bool read_tuple_from_line(table_row_t& tuple, char* buf);

}; // EOF: customer_t



class history_t : public tpcc_table_t 
{
public:
    history_t() : tpcc_table_t("HISTORY", TPCC_HISTORY_FCOUNT) {
        /* table schema */
        _desc[0].setup(SQL_INT,   "H_C_ID");
        _desc[1].setup(SQL_INT,   "H_C_D_ID");  
        _desc[2].setup(SQL_INT,   "H_C_W_ID"); 
        _desc[3].setup(SQL_INT,   "H_D_ID");   
        _desc[4].setup(SQL_INT,   "H_W_ID");    
        _desc[5].setup(SQL_FLOAT, "H_DATE");     /* old: TIME */
        _desc[6].setup(SQL_FLOAT, "H_AMOUNT");   /* old: INT */
        _desc[7].setup(SQL_CHAR,  "H_DATA", 25); 
    }

    bool read_tuple_from_line(table_row_t& tuple, char* buf);

}; // EOF: history_t


class new_order_t : public tpcc_table_t 
{
public:
    new_order_t() : tpcc_table_t("NEW_ORDER", TPCC_NEW_ORDER_FCOUNT) {
        /* table schema */
        _desc[0].setup(SQL_INT, "NO_O_ID");
        _desc[1].setup(SQL_INT, "NO_D_ID");
        _desc[2].setup(SQL_INT, "NO_W_ID");

        /* create unique index no_index on (w_id, d_id, o_id) */
        int keys[3] = {2, 1, 0};
        create_primary_idx("NO_INDEX", keys, 3);

#ifdef CREATE_NOLOCKING_INDEXES
        // last param (nolock) is set to true
        create_index("NO_INDEX_NOLOCK", keys, 3, true, false, true);
#endif
    }

    bool read_tuple_from_line(table_row_t& tuple, char* buf);

}; // EOF: new_order_t


class order_t : public tpcc_table_t 
{    
public:
    order_t() : tpcc_table_t("ORDER", TPCC_ORDER_FCOUNT) {
        /* table schema */
        _desc[0].setup(SQL_INT,   "O_ID");
        _desc[1].setup(SQL_INT,   "O_C_ID");        /* prev [3] */
        _desc[2].setup(SQL_INT,   "O_D_ID");        /* prev [1] */
        _desc[3].setup(SQL_INT,   "O_W_ID");        /* prev [2] */
        _desc[4].setup(SQL_FLOAT, "O_ENTRY_D");     /* old: TIME */
        _desc[5].setup(SQL_INT,   "O_CARRIER_ID"); 
        _desc[6].setup(SQL_INT,   "O_OL_CNT");   
        _desc[7].setup(SQL_INT,   "O_ALL_LOCAL");

        /* create unique index o_index on (w_id, d_id, o_id) */
        int keys1[3] = {3, 2, 0};
        create_index("O_INDEX", keys1, 3);

        /* create unique index o_cust_index on (w_id, d_id, c_id, o_id) */
        int keys2[4] = {3, 2, 1, 0};
        create_index("O_CUST_INDEX", keys2, 4);


#ifdef CREATE_NOLOCKING_INDEXES
        // last param (nolock) is set to true
        create_index("O_INDEX_NOLOCK", keys1, 3, true, false, true);
        create_index("O_CUST_INDEX_NOLOCK", keys2, 4, true, false, true);
#endif
    }

    bool read_tuple_from_line(table_row_t& tuple, char* buf);

}; // EOF: order_t



class order_line_t : public tpcc_table_t 
{
public:
    order_line_t() : tpcc_table_t("ORDERLINE", TPCC_ORDER_LINE_FCOUNT) {
	/* table schema */
	_desc[0].setup(SQL_INT,    "OL_O_ID");
	_desc[1].setup(SQL_INT,    "OL_D_ID");      
	_desc[2].setup(SQL_INT,    "OL_W_ID");   
	_desc[3].setup(SQL_INT,    "OL_NUMBER"); 
	_desc[4].setup(SQL_INT,    "OL_I_ID");
	_desc[5].setup(SQL_INT,    "OL_SUPPLY_W_ID");  
	_desc[6].setup(SQL_FLOAT,  "OL_DELIVERY_D");    /* old: TIME */
	_desc[7].setup(SQL_INT,    "OL_QUANTITY");   
	_desc[8].setup(SQL_INT,    "OL_AMOUNT");
	_desc[9].setup(SQL_CHAR,   "OL_DIST_INFO", 25); /* old: CHAR */  

	/* create unique index ol_index on (w_id, d_id, o_id, ol_number) */
	int keys[4] = {2, 1, 0, 3};
	create_primary_idx("OL_INDEX", keys, 4);
        //	create_index("OL_INDEX", keys, 4, false); /* old: not unique */

#ifdef CREATE_NOLOCKING_INDEXES
        // last param (nolock) is set to true
        create_index("OL_INDEX_NOLOCK", keys, 4, true, false, true);
#endif
    }

    bool read_tuple_from_line(table_row_t& tuple, char* buf);

}; // EOF: order_line_t



class item_t : public tpcc_table_t 
{
public:
    item_t() : tpcc_table_t("ITEM", TPCC_ITEM_FCOUNT) {
	/* table schema */
	_desc[0].setup(SQL_INT,  "I_ID");
	_desc[1].setup(SQL_INT,  "I_IM_ID");
	_desc[2].setup(SQL_CHAR, "I_NAME", 24);   
	_desc[3].setup(SQL_INT,  "I_PRICE");
	_desc[4].setup(SQL_CHAR, "I_DATA", 50);
	
	/* create unique index on i_index on (i_id) */
	int keys[1] = {0};
	create_primary_idx("I_INDEX", keys, 1);

#ifdef CREATE_NOLOCKING_INDEXES
        // last param (nolock) is set to true        
        create_index("I_INDEX_NOLOCK", keys, 1, true, false, true);
#endif
    }

    bool read_tuple_from_line(table_row_t& tuple, char* buf);

}; // EOF: item_t


class stock_t : public tpcc_table_t 
{
public:
    stock_t() : tpcc_table_t("STOCK", TPCC_STOCK_FCOUNT) {
	/* table schema */
	_desc[0].setup(SQL_INT,    "S_I_ID");
	_desc[1].setup(SQL_INT,    "S_W_ID");       
	_desc[2].setup(SQL_INT,    "S_REMOTE_CNT"); /* prev [15] */
	_desc[3].setup(SQL_INT,    "S_QUANTITY");   /* prev [2] */
	_desc[4].setup(SQL_INT,    "S_ORDER_CNT");  /* prev [14] */
	_desc[5].setup(SQL_INT,    "S_YTD");        /* prev [13] */
	_desc[6].setup(SQL_CHAR,   "S_DIST0", 24);  /* prev [4] */
	_desc[7].setup(SQL_CHAR,   "S_DIST1", 24);  
	_desc[8].setup(SQL_CHAR,   "S_DIST2", 24);  
	_desc[9].setup(SQL_CHAR,   "S_DIST3", 24);  
	_desc[10].setup(SQL_CHAR,  "S_DIST4", 24);  
	_desc[11].setup(SQL_CHAR,  "S_DIST5", 24);  
	_desc[12].setup(SQL_CHAR,  "S_DIST6", 24);  
	_desc[13].setup(SQL_CHAR,  "S_DIST7", 24);  
	_desc[14].setup(SQL_CHAR,  "S_DIST8", 24);  
	_desc[15].setup(SQL_CHAR,  "S_DIST9", 24);      /* prev [12] */
        //	_desc[13].setup(SQL_SMALLINT, "S_YTD");        
        //	_desc[14].setup(SQL_SMALLINT, "S_ORDER_CNT");  
        //	_desc[15].setup(SQL_SMALLINT, "S_REMOTE_CNT"); 
	_desc[16].setup(SQL_CHAR,  "S_DATA", 50); 

	/* create unique index s_index on (w_id, i_id) */
	int keys[2] = { 0, 1 };
	create_primary_idx("S_INDEX", keys, 2);

#ifdef CREATE_NOLOCKING_INDEXES
        // last param (nolock) is set to true
        create_index("S_INDEX_NOLOCK", keys, 2, true, false, true);
#endif
    }

    bool read_tuple_from_line(table_row_t& tuple, char* buf);

}; // EOF: stock_t


// loaders
typedef table_loading_smt_impl<warehouse_t>  wh_loader_t;
typedef table_loading_smt_impl<district_t>   dist_loader_t;
typedef table_loading_smt_impl<stock_t>      st_loader_t;
typedef table_loading_smt_impl<order_line_t> ol_loader_t;
typedef table_loading_smt_impl<customer_t>   cust_loader_t;
typedef table_loading_smt_impl<history_t>    hist_loader_t;
typedef table_loading_smt_impl<order_t>      ord_loader_t;
typedef table_loading_smt_impl<new_order_t>  no_loader_t;
typedef table_loading_smt_impl<item_t>       it_loader_t;

// checkers
typedef table_checking_smt_impl<warehouse_t>  wh_checker_t;
typedef table_checking_smt_impl<district_t>   dist_checker_t;
typedef table_checking_smt_impl<stock_t>      st_checker_t;
typedef table_checking_smt_impl<order_line_t> ol_checker_t;
typedef table_checking_smt_impl<customer_t>   cust_checker_t;
typedef table_checking_smt_impl<history_t>    hist_checker_t;
typedef table_checking_smt_impl<order_t>      ord_checker_t;
typedef table_checking_smt_impl<new_order_t>  no_checker_t;
typedef table_checking_smt_impl<item_t>       it_checker_t;



EXIT_NAMESPACE(tpcc);

#endif /* __SHORE_TPCC_SCHEMA_H */
