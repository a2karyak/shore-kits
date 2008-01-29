/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tpcc_schema.h
 *
 *  @brief:  Declaration of the TPC-C tables
 *
 *  @author: Mengzhi Wang, April 2001
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#ifndef __SHORE_TPCC_SCHEMA_H
#define __SHORE_TPCC_SCHEMA_H

#include "sm_vas.h"
#include "util.h"

#include "sm/shore/shore_table.h"

#include "stages/tpcc/shore/shore_tpcc_const.h"
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



/*  --------------------------------------------------------------
 *  @class tpcc_table_t
 *
 *  @brief Base class for all the TPC-C tables.
 *
 *  @note  It simply extends the table_desc_t class with a 
 *         tpcc_random_gen_t member variable 
 *
 *  --------------------------------------------------------------- */

class tpcc_table_t : public table_desc_t {
public:

    tpcc_random_gen_t _tpccrnd;

    tpcc_table_t(const char* name, int fieldcnt)
        : table_desc_t(name, fieldcnt)
    {
        _tpccrnd.reset();
    }

    virtual ~tpcc_table_t() { }

    


}; // EOF: tpcc_table_t




/* -------------------------------------- */
/* all the tables used in tpcc benchmarks */
/* -------------------------------------- */


class warehouse_t : public tpcc_table_t {
public:
    warehouse_t() : tpcc_table_t("WAREHOUSE", TPCC_WAREHOUSE_FCOUNT) {
	/* table schema */
	_desc[0].setup(SQL_SMALLINT, "W_ID");
	_desc[1].setup(SQL_VARCHAR,  "W_NAME", 10);
	_desc[2].setup(SQL_VARCHAR,  "W_STREET1", 20);
	_desc[3].setup(SQL_VARCHAR,  "W_STREET2", 20);
	_desc[4].setup(SQL_VARCHAR,  "W_CITY", 20);   
	_desc[5].setup(SQL_CHAR,     "W_STATE", 2);
	_desc[6].setup(SQL_CHAR,     "W_ZIP", 9); 
	_desc[7].setup(SQL_FLOAT,    "W_TAX");   
	_desc[8].setup(SQL_FLOAT,    "W_YTD");  /* DECIMAL(12,2) */

	/* create unique index w_index on (w_id) */
	int  keys[1] = { 0 };
	create_index("W_INDEX", keys, 1);
    }

    void   random(short w_id);

    w_rc_t  bulkload(ss_m * shore, int w_num);
    
    w_rc_t  index_probe(ss_m * shore, const short  w_id);
    w_rc_t  index_probe_forupdate(ss_m * shore, const short  w_id);
    
    w_rc_t   update_ytd(ss_m * shore,
			const short w_id,
			const double h_amount);
};


class district_t : public tpcc_table_t {
public:
    district_t() : tpcc_table_t("DISTRICT", TPCC_DISTRICT_FCOUNT) {
	/* table schema */
	_desc[0].setup(SQL_SMALLINT, "D_ID");     
	_desc[1].setup(SQL_SMALLINT, "D_W_ID");   
	_desc[2].setup(SQL_CHAR,  "D_NAME", 10);   /* VARCHAR(10) */
	_desc[3].setup(SQL_VARCHAR,  "D_STREET1", 20);
	_desc[4].setup(SQL_VARCHAR,  "D_STREET2", 20);
	_desc[5].setup(SQL_VARCHAR,  "D_CITY", 20);
	_desc[6].setup(SQL_CHAR,     "D_STATE", 2);
	_desc[7].setup(SQL_CHAR,     "D_ZIP", 9); 
	_desc[8].setup(SQL_FLOAT,    "D_TAX");    
	_desc[9].setup(SQL_FLOAT,    "D_YTD");   /* DECIMAL(12,2) */
	_desc[10].setup(SQL_INT,     "D_NEXT_O_ID");

	/* create unique index d_index on (d_id, w_id) */
	int keys[2] = { 0, 1 };
	create_index("D_INDEX", keys, 2);
    }
    
    /* random tuple generator */
    void        random(short id, short w_id, int next_o_id);

    w_rc_t   bulkload(ss_m * shore, int w_num);

    /* --- access the table --- */
    w_rc_t   index_probe(ss_m * shore,
			 const short d_id,
			 const short w_id);
    w_rc_t   index_probe_forupdate(ss_m * shore,
			 const short d_id,
			 const short w_id);

    w_rc_t   update_ytd(ss_m * shore,
			const short d_id,
			const short w_id,
			const double h_amount);

    w_rc_t   update_next_o_id(ss_m * shore,
			      const int  next_o_id);
};


class customer_t : public tpcc_table_t {
public:
    customer_t() : tpcc_table_t("CUSTOMER", TPCC_CUSTOMER_FCOUNT) {
	/* table schema */
	_desc[0].setup(SQL_INT,       "C_ID");         
	_desc[1].setup(SQL_SMALLINT,  "C_D_ID");       
	_desc[2].setup(SQL_SMALLINT,  "C_W_ID");       
	_desc[3].setup(SQL_CHAR,      "C_FIRST", 16);  
	_desc[4].setup(SQL_CHAR,      "C_MIDDLE", 2);  
	_desc[5].setup(SQL_CHAR,      "C_LAST", 16);   
	_desc[6].setup(SQL_VARCHAR,   "C_STREET1", 20);
	_desc[7].setup(SQL_VARCHAR,   "C_STREET2", 20);
	_desc[8].setup(SQL_VARCHAR,   "C_CITY", 20);   
	_desc[9].setup(SQL_CHAR,      "C_STATE", 2);   
	_desc[10].setup(SQL_CHAR,     "C_ZIP", 9);     
	_desc[11].setup(SQL_CHAR,     "C_PHONE", 16);  
	_desc[12].setup(SQL_TIME,     "C_SINCE");      
	_desc[13].setup(SQL_CHAR,     "C_CREDIT", 2);  
	_desc[14].setup(SQL_FLOAT,    "C_CREDIT_LIM");      /* DECIMAL(12,2) */
	_desc[15].setup(SQL_FLOAT,    "C_DISCOUNT");
	_desc[16].setup(SQL_FLOAT,    "C_BALANCE");         /* DECIMAL(12,2) */
	_desc[17].setup(SQL_FLOAT,    "C_YTD_PAYMENT");     /* DECIMAL(12,2) */
	_desc[18].setup(SQL_SMALLINT, "C_PAYMENT_CNT");
	_desc[19].setup(SQL_SMALLINT, "C_DELIVERY_CNT");
	_desc[20].setup(SQL_VARCHAR,  "C_DATA", 500);

	/* create unique index c_index on (w_id, d_id, c_id) */
	int keys1[3] = {2, 1, 0 };
	create_index("C_INDEX", keys1, 3);
	
	/* create index c_name_index on (w_id, d_id, last, first, id) */
	int keys2[5] = {2, 1, 5, 3, 0};
	create_index("C_NAME_INDEX", keys2, 5, false);
    }

    /* random tuple generator */
    void        random(int id, short d_id, short w_id);

    w_rc_t   bulkload(ss_m * shore, int w_num);

    /* index only access */
    w_rc_t   get_iter_by_index(ss_m * shore,
			       index_scan_iter_impl* & iter,
			       const short w_id,
			       const short d_id,
			       const char * c_last);

    w_rc_t   index_probe(ss_m * shore,
			 const int   c_id,
			 const short w_id,
			 const short d_id);
    w_rc_t   index_probe_forupdate(ss_m * shore,
			 const int   c_id,
			 const short w_id,
			 const short d_id);

    w_rc_t   update_tuple(ss_m * shore,
			  const double balance,
			  const double ytd_payment,
			  const short  payment_cnt,
			  const char * data = NULL);
};


class history_t : public tpcc_table_t {
public:
    history_t() : tpcc_table_t("HISTORY", TPCC_HISTORY_FCOUNT) {
	/* table schema */
	_desc[0].setup(SQL_INT,       "H_C_ID");   
	_desc[1].setup(SQL_SMALLINT,  "H_C_D_ID"); 
	_desc[2].setup(SQL_SMALLINT,  "H_C_W_ID"); 
	_desc[3].setup(SQL_SMALLINT,  "H_D_ID");   
	_desc[4].setup(SQL_SMALLINT,  "H_W_ID");   
	_desc[5].setup(SQL_TIME,      "H_DATE");   
	_desc[6].setup(SQL_INT,       "H_AMOUNT"); 
	_desc[7].setup(SQL_VARCHAR,   "H_DATA", 24);
    }

    /* random tuple generator */
    void        random(int c_id, short c_d_id, short c_w_id);
    
    w_rc_t  bulkload(ss_m * shore, int w_num);
};


class  new_order_t : public tpcc_table_t {
public:
    new_order_t() : tpcc_table_t("NEW_ORDER", TPCC_NEW_ORDER_FCOUNT) {
	/* table schema */
	_desc[0].setup(SQL_INT,      "NO_O_ID");
	_desc[1].setup(SQL_SMALLINT, "NO_D_ID");
	_desc[2].setup(SQL_SMALLINT, "NO_W_ID");
	
	/* create unique index no_index on (w_id, d_id, o_id) */
	int keys[3] = {2, 1, 0};
	create_index("NO_INDEX", keys, 3);
    }

    /* random tuple generator */
    void        random(int id, short d_id, short w_id);

    w_rc_t  bulkload(ss_m * shore, int w_num);

    w_rc_t  get_iter_by_index(ss_m * shore,
			      index_scan_iter_impl* &iter,
			      const short w_id,
			      const short d_id);

    w_rc_t  delete_by_index(ss_m * shore,
			    const short  w_id,
			    const short  d_id,
			    const int    o_id);
};


class order_t : public tpcc_table_t {
public:
    order_t() : tpcc_table_t("ORDERS", TPCC_ORDER_FCOUNT) {
	/* table schema */
	_desc[0].setup(SQL_INT,      "O_ID");    
	_desc[1].setup(SQL_SMALLINT, "O_D_ID");  
	_desc[2].setup(SQL_SMALLINT, "O_W_ID");  
	_desc[3].setup(SQL_INT,      "O_C_ID");  
	_desc[4].setup(SQL_TIME,     "O_ENTRY_D");    
	_desc[5].setup(SQL_SMALLINT, "O_CARRIER_ID"); 
	_desc[6].setup(SQL_SMALLINT, "O_OL_CNT");   
	_desc[7].setup(SQL_SMALLINT, "O_ALL_LOCAL");
	
	/* create unique index o_index on (w_id, d_id, o_id) */
	int keys1[3] = {2, 1, 0};
	create_index("O_INDEX", keys1, 3);

	/* create unique index o_cust_index on (w_id, d_id, c_id, o_id) */
	int keys2[4] = {2, 1, 3, 0};
	create_index("O_CUST_INDEX", keys2, 4);
    }

    /* random tuple generator */
    void        random(int id, int c_id, short d_id, short w_id, short ol_cnt);

    w_rc_t  bulkload(ss_m * shore, int w_num, short *);

    w_rc_t  update_carrier_by_index(ss_m * shore,
				    const short w_id,
				    const short d_id,
				    const int   o_id,
				    const short carrier_id);

    w_rc_t  get_iter_by_index(ss_m * shore,
			      index_scan_iter_impl* & iter,
			      const short w_id,
			      const short d_id,
			      const int   c_id);
};


class order_line_t : public tpcc_table_t {
public:
    order_line_t() : tpcc_table_t("ORDER_LINE", TPCC_ORDER_LINE_FCOUNT) {
	/* table schema */
	_desc[0].setup(SQL_INT,      "OL_O_ID");   
	_desc[1].setup(SQL_SMALLINT, "OL_D_ID");   
	_desc[2].setup(SQL_SMALLINT, "OL_W_ID");   
	_desc[3].setup(SQL_SMALLINT, "OL_NUMBER"); 
	_desc[4].setup(SQL_INT,      "0L_I_ID");   
	_desc[5].setup(SQL_SMALLINT, "OL_SUPPLY_W_ID");   
	_desc[6].setup(SQL_TIME,     "OL_DELIVERY_D", 0, true);
	_desc[7].setup(SQL_SMALLINT, "OL_QUANTITY");   
	_desc[8].setup(SQL_INT,      "OL_AMOUNT");     
	_desc[9].setup(SQL_CHAR,     "OL_DIST_INFO", 24);  

	/* create unique index ol_index on (w_id, d_id, o_id, ol_number) */
	int keys[4] = {2, 1, 0, 3};
	create_index("OL_INDEX", keys, 4, false);
    }

    /* random tuple generator */
    void        random(int id, short d_id, short w_id,
		       short ol_index, bool delivery=true);

    w_rc_t  bulkload(ss_m * shore, int w_num, short *);

    /* --- access methods --- */
    w_rc_t  get_iter_by_index(ss_m * shore,
			      index_scan_iter_impl* & it,
			      const short w_id,
			      const short d_id,
			      const int   low_o_id,
			      const int   high_o_id);

    w_rc_t  get_iter_by_index(ss_m * shore,
			      index_scan_iter_impl* & iter,
			      const short w_id,
			      const short d_id,
			      const int   o_id);
};


class item_t : public tpcc_table_t {
public:
    item_t() : tpcc_table_t("ITEM", TPCC_ITEM_FCOUNT) {
	/* table schema */
	_desc[0].setup(SQL_INT,     "I_ID");         
	_desc[1].setup(SQL_INT,     "I_IM_ID");      
	_desc[2].setup(SQL_VARCHAR, "I_NAME", 24);   
	_desc[3].setup(SQL_INT,     "I_PRICE");      
	_desc[4].setup(SQL_VARCHAR, "I_DATA", 50);
	
	/* create unique index on i_index on (i_id) */
	int keys[1] = {0};
	create_index("I_INDEX", keys, 1);
    }

    /* random tuple generator */
    void        random(int id);

    w_rc_t  bulkload(ss_m * shore);

    w_rc_t  index_probe(ss_m * shore, const int i_id);
    w_rc_t  index_probe_forupdate(ss_m * shore, const int i_id);
};


class stock_t : public tpcc_table_t {
public:
    stock_t() : tpcc_table_t("STOCK", TPCC_STOCK_FCOUNT) {
	/* table schema */
	_desc[0].setup(SQL_INT,      "S_I_ID");       
	_desc[1].setup(SQL_SMALLINT, "S_W_ID");      
	_desc[2].setup(SQL_SMALLINT, "S_QUANTITY");   
	_desc[3].setup(SQL_CHAR,     "S_DIST0", 24);  
	_desc[4].setup(SQL_CHAR,     "S_DIST1", 24);  
	_desc[5].setup(SQL_CHAR,     "S_DIST2", 24);  
	_desc[6].setup(SQL_CHAR,     "S_DIST3", 24);  
	_desc[7].setup(SQL_CHAR,     "S_DIST4", 24);  
	_desc[8].setup(SQL_CHAR,     "S_DIST5", 24);  
	_desc[9].setup(SQL_CHAR,     "S_DIST6", 24);  
	_desc[10].setup(SQL_CHAR,    "S_DIST7", 24);  
	_desc[11].setup(SQL_CHAR,    "S_DIST8", 24);  
	_desc[12].setup(SQL_CHAR,    "S_DIST9", 24);  
	_desc[13].setup(SQL_INT,     "S_YTD");        
	_desc[14].setup(SQL_SMALLINT,"S_ORDER_CNT");  
	_desc[15].setup(SQL_SMALLINT,"S_REMOTE_CNT"); 
	_desc[16].setup(SQL_VARCHAR, "S_DATA", 50); 

	/* create unique index s_index on (w_id, i_id) */
	int keys[2] = { 0, 1 };
	create_index("S_INDEX", keys, 2);
    }

    /* random tuple generator */
    void        random(int id, short w_id);

    w_rc_t  bulkload(ss_m * shore, int w_num);

    w_rc_t  index_probe(ss_m * shore,
			const int   i_id,
			const short w_id);
    w_rc_t  index_probe_forupdate(ss_m * shore,
			const int   i_id,
			const short w_id);

    w_rc_t  update_tuple(ss_m * shore,
			 const short  quantity,
			 const short  order_cnt,
			 const double ytd,
			 const short  remote_cnt);
};


EXIT_NAMESPACE(tpcc);

#endif /* __SHORE_TPCC_SCHEMA_H */
