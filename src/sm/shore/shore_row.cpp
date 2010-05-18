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

/** @file:   shore_row.cpp
 *
 *  @brief:  Implementation of the base class for records (rows) of tables in Shore
 *
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#include "sm/shore/shore_row.h"



// #include <strstream>
// char const* db_pretty_print(shore::table_row_t const* rec, int /* i=0 */, char const* /* s=0 */)
// {
//     static char data[1024];
//     std::strstream inout(data, sizeof(data));
//     ((shore::table_row_t*)rec)->print_values(inout);
//     inout << std::ends;
//     return data;
// }

#include <sstream>
char const* db_pretty_print(shore::table_row_t const* rec, int /* i=0 */, char const* /* s=0 */)
{
    static char data[1024];
    std::stringstream inout(data,stringstream::in | stringstream::out);
    //std::strstream inout(data, sizeof(data));
    ((shore::table_row_t*)rec)->print_values(inout);
    inout << std::ends;
    return data;
}


ENTER_NAMESPACE(shore);


/******************************************************************
 * 
 * struct rep_row_t methods 
 *
 ******************************************************************/



/******************************************************************
 * 
 * @fn:    set
 *
 * @brief: Set new buffer size
 *
 ******************************************************************/

void rep_row_t::set(const uint nsz)
{
    if ((!_dest) || (_bufsz < nsz)) {

        char* tmp = _dest;

        // Using the trash stack
        assert (_pts);
        _dest = new(*_pts) char(nsz);

        if (tmp) {
            //            delete [] tmp;
            _pts->destroy(tmp);
            tmp = NULL;
        } 
        _bufsz = _pts->nbytes();
    }

    // in any case, clean up the buffer
    memset (_dest, 0, nsz);
}


/******************************************************************
 * 
 * @fn:    set_ts
 *
 * @brief: Set new trash stack and buffer size
 *
 ******************************************************************/

void rep_row_t::set_ts(ats_char_t* apts, const uint nsz)
{
    assert(apts);
    _pts = apts;
    set(nsz);
}



/******************************************************************
 * 
 * class table_row_t methods 
 *
 ******************************************************************/


/****************************************************************** 
 *
 *  @fn:    size()
 *
 *  @brief: Return the actual size of the tuple in disk format
 *
 ******************************************************************/

uint table_row_t::size() const
{
    assert (_is_setup);

    uint size = 0;

    /* for a fixed length field, it just takes as much as the
     * space for the value itself to store.
     * for a variable length field, we store as much as the data
     * and the offset to tell the length of the data.
     * Of course, there is a bit for each nullable field.
     */

    for (uint i=0; i<_field_cnt; i++) {
	if (_pvalues[i]._pfield_desc->allow_null()) {
	    if (_pvalues[i].is_null()) continue;
	}
	if (_pvalues[i].is_variable_length()) {
	    size += _pvalues[i].realsize();
	    size += sizeof(offset_t);
	}
	else size += _pvalues[i].maxsize();
    }
    if (_null_count) size += (_null_count >> 3) + 1;
    return (size);
}




/* ----------------- */
/* --- debugging --- */
/* ----------------- */

/* For debug use only: print the value of all the fields of the tuple */
void table_row_t::print_values(ostream& os)
{
    assert (_is_setup);
    //  cout << "Number of fields: " << _field_count << endl;
    for (uint i=0; i<_field_cnt; i++) {
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
    for (uint i=0; i<_field_cnt; i++) {
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
    for (uint i=0; i<_field_cnt; i++) {
        sz = _pvalues[i].get_debug_str(sbuf);
        if (sbuf) {
            fprintf( stderr, "%d. %s (%d)\n", i, sbuf, sz);
            delete [] sbuf;
            sbuf = NULL;
        }
    }
}


EXIT_NAMESPACE(shore);


