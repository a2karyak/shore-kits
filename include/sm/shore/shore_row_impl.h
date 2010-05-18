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

/** @file:   shore_row_impl.h
 *
 *  @brief:  Base class for tuples used in Shore
 *
 *  @note:   row_impl             - class for tuple-related operations
 *
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#ifndef __SHORE_ROW_IMPL_H
#define __SHORE_ROW_IMPL_H

#include "shore_row.h"


ENTER_NAMESPACE(shore);



/* ---------------------------------------------------------------
 *
 * @struct: tuple_impl
 *
 * @brief:  Template-based class for representing a row (record) of
 *          a Shore table. 
 *
 * --------------------------------------------------------------- */

template <class TableDesc>
struct row_impl : public table_row_t 
{    
    TableDesc*  _ptable;   /* pointer back to the table description */


    /* -------------------- */
    /* --- construction --- */
    /* -------------------- */

    row_impl(TableDesc* ptd)
    {
        assert (ptd);
        setup(ptd);
    }

    row_impl()
        : _ptable(NULL)
    {
    }
        
    ~row_impl() 
    {
    }


    /* ----------------------------------------------------------------- */
    /* --- setup row according to table description, asserts if NULL --- */
    /* ----------------------------------------------------------------- */

    int setup(TableDesc* ptd) 
    {
        assert (ptd);

        // if it is already setup for this table just reset it
        if ((_ptable == ptd) && (_pvalues != NULL) && (_is_setup)) {
            reset();
            return (1);
        }

        // else do the normal setup
        _ptable = ptd;
        _field_cnt = ptd->field_count();
        assert (_field_cnt>0);
        _pvalues = new field_value_t[_field_cnt];

        uint var_count  = 0;
        uint fixed_size = 0;

        // setup each field and calculate offsets along the way
        for (uint i=0; i<_field_cnt; i++) {
            _pvalues[i].setup(ptd->desc(i));

            // count variable- and fixed-sized fields
            if (_pvalues[i].is_variable_length())
                var_count++;
            else
                fixed_size += _pvalues[i].maxsize();

            // count null-able fields
            if (_pvalues[i].field_desc()->allow_null())
                _null_count++;            
        }

        // offset for fixed length field values
        _fixed_offset = 0;
        if (_null_count) _fixed_offset = ((_null_count-1) >> 3) + 1;
        // offset for variable length field slots
        _var_slot_offset = _fixed_offset + fixed_size; 
        // offset for variable length field values
        _var_offset = _var_slot_offset + sizeof(offset_t)*var_count;

        _is_setup = true;
        return (0);
    }

}; // EOF: row_impl



EXIT_NAMESPACE(shore);

#endif /* __SHORE_ROW_IMPL_H */
