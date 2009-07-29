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
 *  @brief:  Declaration of the TPC-B tables
 *
 *  @author: Ryan Johnson      (ryanjohn)
 *  @author: Ippokratis Pandis (ipandis)
 *  @date:   Feb 2009
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

        int  keys1[1] = { 0 }; // IDX { B_ID }

        // baseline - Regular indexes
        if (sysname.compare("baseline")==0) {
            TRACE( TRACE_DEBUG, "Regular idxs for (%s)\n", _name);
	
            // create unique index b_index on (b_id)
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

        int keys1[1] = { 0 }; // IDX { T_ID }

        // baseline - Regular indexes
        if (sysname.compare("baseline")==0) {
            TRACE( TRACE_DEBUG, "Regular idxs for (%s)\n", _name);
	
            // create unique index t_index on (t_id)
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
    }
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

        // baseline - Regular indexes
        if (sysname.compare("baseline")==0) {
            TRACE( TRACE_DEBUG, "Regular idxs for (%s)\n", _name);
	
            // create unique index t_index on (a_id)
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
    }
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
        _desc[3].setup(SQL_FLOAT, "H_DELTA");   
        _desc[4].setup(SQL_FLOAT, "H_TIME");        
        _desc[5].setup(SQL_CHAR,  "H_PADDING", 50-3*sizeof(int)-2*sizeof(double)); 

        // NO INDEXES
    }
}; // EOF: history_t



EXIT_NAMESPACE(tpcb);

#endif /* __SHORE_TPCB_SCHEMA_H */
