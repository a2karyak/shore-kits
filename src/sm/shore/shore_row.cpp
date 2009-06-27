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

/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_row.cpp
 *
 *  @brief Implementation of table_row_t 
 *
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#include "sm/shore/shore_row.h"

using namespace shore;


/* ----------------- */
/* --- debugging --- */
/* ----------------- */

/* For debug use only: print the value of all the fields of the tuple */
void table_row_t::print_values(ostream& os)
{
    assert (_is_setup);
    for (int i=0; i<_field_cnt; i++) {
	_pvalues[i].print_value(os);
	if (i != _field_cnt) os << DELIM_CHAR;
    }
    os << endl;
}



/* For debug use only: print the tuple */
void table_row_t::print_tuple()
{
    assert (_is_setup);
    
    char* sbuf = NULL;
    int sz = 0;
    for (int i=0; i<_field_cnt; i++) {
        sz = _pvalues[i].get_debug_str(sbuf);
        if (sbuf) {
            TRACE( TRACE_TRX_FLOW, "%d. %s (%d)\n", i, sbuf, sz);
            delete [] sbuf;
            sbuf = NULL;
        }
    }
}


/* For debug use only: print the tuple without tracing */
void table_row_t::print_tuple_no_tracing()
{
    assert (_is_setup);
    
    char* sbuf = NULL;
    int sz = 0;
    for (int i=0; i<_field_cnt; i++) {
        sz = _pvalues[i].get_debug_str(sbuf);
        if (sbuf) {
            fprintf( stderr, "%d. %s (%d)\n", i, sbuf, sz);
            delete [] sbuf;
            sbuf = NULL;
        }
    }
}


#include <strstream>
char const* db_pretty_print(table_row_t const* rec, int i=0, char const* s=0) {
    static char data[1024];
    std::strstream inout(data, sizeof(data));
    ((table_row_t*)rec)->print_values(inout);
    inout << std::ends;
    return data;
}

