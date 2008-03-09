/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_row_impl.h
 *
 *  @brief:  Base class for tuples used in Shore
 *
 *  @note:   row_impl             - class for tuple-related operations
 *
 *  @author: Ippokratis Pandis, January 2008
 *
 */

/* shore_row_impl.h contains the template-based base class (tuple_impl) 
 * for operations on tuples stored in Shore. 
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
        if ((_ptable == ptd) && (_is_setup)) {
            reset();
            return (1);
        }

//         if (_pvalues) {
//             delete [] _pvalues;
//             _pvalues = NULL;
//         }

        // else do the normal setup
        _ptable = ptd;
        _field_cnt = ptd->field_count();
        assert (_field_cnt>0);
        _pvalues = new field_value_t[_field_cnt];
        for (int i=0; i<_field_cnt; i++)
            _pvalues[i].setup(ptd->desc(i));

        _is_setup = true;
        return (0);
    }

}; // EOF: row_impl



EXIT_NAMESPACE(shore);

#endif /* __SHORE_ROW_IMPL_H */
