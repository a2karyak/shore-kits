/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tpcb_schema.h
 *
 *  @brief:  Declaration of the TPC-C tables
 *
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#ifndef __SHORE_TPCB_SCHEMA_H
#define __SHORE_TPCB_SCHEMA_H

#include <math.h>

#include "sm_vas.h"
#include "util.h"

#include "sm/shore/shore_table_man.h"


using namespace shore;


ENTER_NAMESPACE(tpcb);



/*********************************************************************
 *
 * TPC-B SCHEMA
 * 
 * This file contains the classes for tables in tpcb benchmark.
 * A class derived from tpcb_table_t (which inherits from table_desc_t) 
 * is created for each table in the databases.
 *
 *********************************************************************/


/*
 * A primary index is created on each table except HISTORY
 */


typedef std::list<table_desc_t*> tpcb_table_desc_list;
typedef std::list<table_desc_t*>::iterator tpcb_table_desc_list_iter;


/* -------------------------------------------------- */
/* --- All the tables used in the TPC-C benchmark --- */
/* -------------------------------------------------- */


class branch_t : public table_desc_t 
{
public:
    branch_t(string sysname) : 
        table_desc_t("BRANCH", 3) 
    {
        /* table schema */
        _desc[0].setup(SQL_INT,   "B_ID");
        _desc[1].setup(SQL_FLOAT, "B_BALANCE");
        _desc[2].setup(SQL_CHAR,  "B_PADDING", 100-sizeof(int)-sizeof(double));

        int  keys[1] = { 0 }; // IDX { B_ID }

        // depending on the system name, create the corresponding indexes 
        int idxs_created = 0;

        // baseline - regular indexes
	assert (idxs_created==0);
	idxs_created=1;
	TRACE( TRACE_DEBUG, "Regular idxs for (%s)\n", _name);
	
	// create unique index w_index on (w_id)
	create_primary_idx("B_INDEX", 0, keys, 1);
        assert (idxs_created==1); // make sure that idxs were created
    }
    bool read_tuple_from_line(table_row_t& tuple, char* buf) { return false; }
}; // EOF: branch_t



class teller_t : public table_desc_t 
{
public:
    teller_t(string sysname) : 
        table_desc_t("TELLER", 4) 
    {
        /* table schema */
        _desc[0].setup(SQL_INT,   "T_ID");     
        _desc[1].setup(SQL_INT,   "T_B_ID");   
        _desc[2].setup(SQL_FLOAT, "T_BALANCE");
        _desc[3].setup(SQL_CHAR,  "T_PADDING", 100-2*sizeof(int) - sizeof(double));

        int keys[1] = { 0 }; // IDX { T_ID }

        // depending on the system name, create the corresponding indexes 
        int idxs_created = 0;

	assert (idxs_created==0);
	idxs_created=1;
	TRACE( TRACE_DEBUG, "Regular idxs for (%s)\n", _name);
	
	// create unique index d_index on (d_id, w_id)
	create_primary_idx("T_INDEX", 0, keys, 1);
        assert (idxs_created==1); // make sure that idxs were created
    }

    bool read_tuple_from_line(table_row_t& tuple, char* buf) { return false; }
}; // EOF: teller_t



class account_t : public table_desc_t 
{
public:
    account_t(string sysname) : 
        table_desc_t("ACCOUNT", 4) 
    {
        /* table schema */
        _desc[0].setup(SQL_INT,    "A_ID");
        _desc[1].setup(SQL_INT,    "A_B_ID");       
        _desc[2].setup(SQL_FLOAT,  "A_BALANCE");  
        _desc[3].setup(SQL_CHAR,   "A_PADDING", 100-2*sizeof(int)-sizeof(double));  

        int keys1[1] = {0 }; // IDX { A_ID }


        // depending on the system name, create the corresponding indexes 
        int idxs_created = 0;

        // baseline - regular indexes
            assert (idxs_created==0);
            idxs_created=1;
            TRACE( TRACE_DEBUG, "Regular idxs for (%s)\n", _name);

            // create unique index c_index on (w_id, d_id, c_id)
            create_primary_idx("C_INDEX", 0, keys1, 1);
        assert (idxs_created==1); // make sure that idxs were created
    }

    bool read_tuple_from_line(table_row_t& tuple, char* buf) { return false; }
}; // EOF: account_t



class history_t : public table_desc_t 
{
public:
    history_t(string sysname) : 
        table_desc_t("HISTORY", 6) 
    {
        /* table schema */
        _desc[0].setup(SQL_INT,   "H_B_ID");
        _desc[1].setup(SQL_INT,   "H_T_ID");  
        _desc[2].setup(SQL_INT,   "H_A_ID"); 
        _desc[3].setup(SQL_FLOAT, "H_DELTA");   /* old: INT */
        _desc[4].setup(SQL_FLOAT, "H_TIME");     /* old: TIME */
        _desc[5].setup(SQL_CHAR,  "H_PADDING", 50-3*sizeof(int)-2*sizeof(double)); 

        // NO INDEXES
    }

    bool read_tuple_from_line(table_row_t& tuple, char* buf) { return false; }
}; // EOF: history_t



// loaders
typedef table_loading_smt_impl<branch_t>  b_loader_t;
typedef table_loading_smt_impl<teller_t>   t_loader_t;
typedef table_loading_smt_impl<account_t>      a_loader_t;
typedef table_loading_smt_impl<history_t>    hist_loader_t;

// checkers
typedef table_checking_smt_impl<branch_t>  b_checker_t;
typedef table_checking_smt_impl<teller_t>   t_checker_t;
typedef table_checking_smt_impl<account_t>   a_checker_t;
typedef table_checking_smt_impl<history_t>    hist_checker_t;



EXIT_NAMESPACE(tpcb);

#endif /* __SHORE_TPCB_SCHEMA_H */
