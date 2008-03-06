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
    //  cout << "Number of fields: " << _field_count << endl;
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

