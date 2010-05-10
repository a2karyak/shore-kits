/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
      Data Intensive Applications and Systems Labaratory (DIAS)
               Ecole Polytechnique Federale de Lausanne
   
                         All Rights Reserved.
   
   Permission to use, copy, modify and distribute this software and
   its documentation is hereby granted, provided that both the
   copyright notice and this permission notice appear in all copies of
   the software, derivative works or modified versions, and any
   portions thereof, and that both notices appear in supporting
   documentation.
   
   This code is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
   DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
   RESULTING FROM THE USE OF THIS SOFTWARE.
*/

/** @file:   shore_tpcb_schema.h
 *
 *  @brief:  Declaration of the TPC-C tables
 *
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#include "workload/tpcb/shore_tpcb_schema.h"

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
 *
 * 1. BRANCH
 * a. primary (unique) index on branch(b_id)
 * 
 * 2. TELLER
 * a. primary (unique) index on teller(t_id)
 *
 * 3. ACCOUNT
 * a. primary (unique) index on account(a_id)
 *
 */


branch_t::branch_t(string sysname) : 
        table_desc_t("BRANCH", 3) 
{
    /* table schema */
    _desc[0].setup(SQL_INT,   "B_ID");
    _desc[1].setup(SQL_FLOAT, "B_BALANCE");
    _desc[2].setup(SQL_FIXCHAR,  "B_PADDING", 100-sizeof(int)-sizeof(double));
    
    int  keys1[1] = { 0 }; // IDX { B_ID }
    
    if (sysname.compare("baseline")==0) {
	TRACE( TRACE_DEBUG, "Regular idxs for (%s)\n", _name);
	
	// create unique index w_index on (b_id)
	create_primary_idx("B_INDEX", 0, keys1, 1);
    }

#ifdef CFG_DORA
        // dora - NL indexes
        if (sysname.compare("dora")==0) {
            TRACE( TRACE_DEBUG, "NoLock idxs for (%s)\n", _name);
        
            // create unique index s_index on (s_id)
            // last param (nolock) is set to true
            create_primary_idx("B_IDX_NL", 0, keys1, 1, true);
        }       
#endif

    }


teller_t::teller_t(string sysname) : 
        table_desc_t("TELLER", 4) 
{
    /* table schema */
    _desc[0].setup(SQL_INT,   "T_ID");     
    _desc[1].setup(SQL_INT,   "T_B_ID");   
    _desc[2].setup(SQL_FLOAT, "T_BALANCE");
    _desc[3].setup(SQL_FIXCHAR,  "T_PADDING", 100-2*sizeof(int) - sizeof(double));
    
    int keys1[1] = { 0 }; // IDX { T_ID }

    // baseline - Regular indexes
    if (sysname.compare("baseline")==0) {    
	TRACE( TRACE_DEBUG, "Regular idxs for (%s)\n", _name);
	
	// create unique index d_index on (d_id, w_id)
	create_primary_idx("T_INDEX", 0, keys1, 1);
    }

#ifdef CFG_DORA
        // dora - NL indexes
        if (sysname.compare("dora")==0) {
            TRACE( TRACE_DEBUG, "NoLock idxs for (%s)\n", _name);
        
            // create unique index s_index on (t_id)
            // last param (nolock) is set to true
            create_primary_idx("T_IDX_NL", 0, keys1, 1, true);
        }       
#endif

}; // EOF: teller_t


account_t::account_t(string sysname) : 
        table_desc_t("ACCOUNT", 4) 
{
    /* table schema */
    _desc[0].setup(SQL_INT,    "A_ID");
    _desc[1].setup(SQL_INT,    "A_B_ID");       
    _desc[2].setup(SQL_FLOAT,  "A_BALANCE");  
    _desc[3].setup(SQL_FIXCHAR,   "A_PADDING", 100-2*sizeof(int)-sizeof(double));  
    
    int keys1[1] = {0 }; // IDX { A_ID }


    // depending on the system name, create the corresponding indexes 
    int idxs_created = 0;

    // baseline - regular indexes
    if (sysname.compare("baseline")==0) {
        TRACE( TRACE_DEBUG, "Regular idxs for (%s)\n", _name);

        // create unique index c_index on (w_id, d_id, c_id)
        create_primary_idx("A_INDEX", 0, keys1, 1);
    }

#ifdef CFG_DORA
        // dora - NL indexes
        if (sysname.compare("dora")==0) {
            TRACE( TRACE_DEBUG, "NoLock idxs for (%s)\n", _name);
        
            // create unique index s_index on (a_id)
            // last param (nolock) is set to true
            create_primary_idx("A_IDX_NL", 0, keys1, 1, true);
        }       
#endif

}; // EOF: account_t


history_t::history_t(string sysname) : 
        table_desc_t("HISTORY", 6) 
    {
        /* table schema */
        _desc[0].setup(SQL_INT,   "H_B_ID");
        _desc[1].setup(SQL_INT,   "H_T_ID");  
        _desc[2].setup(SQL_INT,   "H_A_ID"); 
        _desc[3].setup(SQL_FLOAT, "H_DELTA");   /* old: INT */
        _desc[4].setup(SQL_FLOAT, "H_TIME");     /* old: TIME */
        _desc[5].setup(SQL_FIXCHAR,  "H_PADDING", 50-3*sizeof(int)-2*sizeof(double)); 

        // NO INDEXES
    }


EXIT_NAMESPACE(tpcb);
