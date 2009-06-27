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

/** @file:   shore_row.h
 *
 *  @brief:  Base class for rows of tables stored in Shore
 *
 *  @note:   tuple_row_t - row of a table
 *
 *  @author: Ippokratis Pandis, January 2008
 *
 */


/* shore_row.h contains the (abstract) base class (tuple_row_t) for the 
 * tuple representation. 
 *
 *
 * FUNCTIONALITY
 *
 * There are methods for formatting a tuple to its disk representation, 
 * and loading it to memory, as well as, methods for accessing
 * the various fields of the tuple.
 *
 *
 * BUGS:
 *
 * Timestamp field is not fully implemented: no set function.
 *
 *
 * EXTENSIONS:
 *
 * The mapping between SQL types and C++ types are defined in
 * (field_desc_t).  Modify the class to support more SQL types or
 * change the mapping.  The NUMERIC type is currently stored as string;
 * no further understanding is provided yet.  
 *
 */

/* The disk format of the record looks like this:
 *
 *  +--+-----+------------+-+-+-----+-------------+
 *  |NF| a   | d          | | | b   | c           |
 *  +--+-----+------------+-+-+-----+-------------+
 *                         | |      ^             ^
 *                         | +------+-------------+
 *                         |        |
 *                         +--------+
 *
 * The first part of tuple (NF) is dedicated to the null flags.  There
 * is a bit for each nullable field in null flags to tell whether the
 * data is presented. The space for the null flag is rounded to bytes.
 *
 * All the fixed size fields go first (a and d) and variable length
 * fields are appended after that. (b and c).  For the variable length
 * fields, we don't reserve the space for the full length of the value
 * but allocate as much space as needed.  Therefore, we need offsets
 * to tell the length of the actual values.  So here comes the two
 * additional slots in the middle (between d and b).  In our
 * implementation, we store the offset of the end of b relative to the
 * beginning of the tuple (address of a).  
 *
 */

#ifndef __SHORE_ROW_H
#define __SHORE_ROW_H


#include "util.h"
#include "atomic_trash_stack.h"

#include "shore_field.h"



ENTER_NAMESPACE(shore);


/* ---------------------------------------------------------------
 *
 * @struct: rep_row_t
 *
 * @brief:  A simple structure with a pointer to a buffer and its
 *          corresponding size.
 *
 * @note:   Not thread-safe, the caller should regulate access.
 *
 * --------------------------------------------------------------- */

typedef atomic_class_stack<char> ats_char_t;

struct rep_row_t 
{    
    char* _dest;       /* pointer to a buffer */
    int   _bufsz;      /* buffer size */
    ats_char_t* _pts;  /* pointer to a trash stack */

    rep_row_t(ats_char_t* apts) 
        : _dest(NULL), _bufsz(0), _pts(apts)
    { 
        assert (_pts);
    }

    rep_row_t(char* &dest, int &bufsz, ats_char_t* apts) 
        : _dest(dest), _bufsz(0), _pts(apts)
    {
        assert (_pts);
    }

    ~rep_row_t() {
        if (_dest) {
            //            delete [] _dest;
            _pts->destroy(_dest);
            _dest = NULL;
        }
    }

    void set(const int new_bufsz);

    //    static int _static_format_mallocs;

}; // EOF: rep_row_t



/* ---------------------------------------------------------------
 *
 * @abstract struct: table_row_t
 *
 * @brief:  Abstract base class for the representation a row (record) 
 *          of a table. 
 *
 * --------------------------------------------------------------- */

#ifndef __SUNPRO_CC
typedef intptr_t offset_t;
#endif

struct table_row_t 
{    
    int            _field_cnt;    /* number of fields */
    bool           _is_setup;     /* flag if already setup */
    
    rid_t          _rid;          /* record id */    
    field_value_t* _pvalues;      /* set of values */

    // pre-calculated offsets
    offset_t _fixed_offset;
    offset_t _var_slot_offset;
    offset_t _var_offset;
    int      _null_count;


    rep_row_t*     _rep;          /* a pointer to a row representation struct */


    /* -------------------- */
    /* --- construction --- */
    /* -------------------- */

    table_row_t() 
        : _field_cnt(0), _is_setup(false), 
          _rid(rid_t::null), _pvalues(NULL), 
          _fixed_offset(0),_var_slot_offset(0),_var_offset(0),_null_count(0),
          _rep(NULL)
    { 
    }
        
    virtual ~table_row_t() 
    {
        freevalues();
    }

    /* ---------------------- */
    /* --- access methods --- */
    /* ---------------------- */

    inline rid_t rid() const { return (_rid); }
    inline void  set_rid(const rid_t& rid) { _rid = rid; }
    inline const bool  is_rid_valid() const { return (_rid != rid_t::null); }

    inline const offset_t get_fixed_offset() const { return (_fixed_offset); }
    inline const offset_t get_var_slot_offset() const { return (_var_slot_offset); }
    inline const offset_t get_var_offset() const { return (_var_offset); }
    inline const int get_null_count() const { return (_null_count); }

    const int size() const;


    /* ------------------------ */
    /* --- set field values --- */
    /* ------------------------ */
    
    void set_null(const int idx);
    void set_value(const int idx, const int v);
    void set_value(const int idx, const bool v);
    void set_value(const int idx, const short v);
    void set_value(const int idx, const double v);
    void set_value(const int idx, const decimal v);
    void set_value(const int idx, const time_t v);
    void set_value(const int idx, const char* string);
    void set_value(const int idx, const timestamp_t& time);


    /* ------------------------ */
    /* --- get field values --- */
    /* ------------------------ */

    bool get_value(const int idx, int& value) const;
    bool get_value(const int idx, bool& value) const;
    bool get_value(const int idx, short& value) const;
    bool get_value(const int idx, char* buffer, const int bufsize) const;
    bool get_value(const int idx, double& value) const;
    bool get_value(const int idx, decimal& value) const;
    bool get_value(const int idx, time_t& value) const;
    bool get_value(const int idx, timestamp_t& value) const;


    /* ----------------- */
    /* --- debugging --- */
    /* ----------------- */

    void print_values(ostream& os = cout); /* print the tuple values */
    void print_tuple();                    /* print the whole tuple */
    void print_tuple_no_tracing();         /* print the whole tuple without trace msg */


    /* ------------------------------ */
    /* --- required functionality --- */
    /* ------------------------------ */

    //    virtual void reset()=0; /* clear the tuple and prepare it for re-use */

    /* clear the tuple and prepare it for re-use */
    void reset() { 
        assert (_is_setup);
        //        assert (_ptable);
        for (int i=0; i<_field_cnt; i++)
            _pvalues[i].reset();
    }        

    void freevalues()
    {
        if (_pvalues) {
            delete [] _pvalues;
            _pvalues = NULL;
        }
    }



}; // EOF: table_row_t



/******************************************************************
 * 
 * struct rep_row_t methods 
 *
 ******************************************************************/

inline void rep_row_t::set(const int new_bufsz)
{
    if ((!_dest) || (_bufsz < new_bufsz)) {

        char* tmp = _dest;

        // Using the trash stack
        assert (_pts);
        _dest = new(*_pts) char(new_bufsz);

        if (tmp) {
            //            delete [] tmp;
            _pts->destroy(tmp);
            tmp = NULL;
        } 
        _bufsz = _pts->nbytes();
    }

    // in any case, clean up the buffer
    memset (_dest, 0, new_bufsz);
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

inline const int table_row_t::size() const
{
    assert (_is_setup);

    int size = 0;

    /* for a fixed length field, it just takes as much as the
     * space for the value itself to store.
     * for a variable length field, we store as much as the data
     * and the offset to tell the length of the data.
     * Of course, there is a bit for each nullable field.
     */

    for (int i=0; i<_field_cnt; i++) {
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



/******************************************************************
 * 
 * SET value functions
 *
 ******************************************************************/
    
inline void table_row_t::set_null(const int idx) 
{
    assert (_is_setup);
    assert (idx >= 0 && idx < _field_cnt);
    assert (_pvalues[idx].is_setup());
    _pvalues[idx].set_null();
}

inline void table_row_t::set_value(const int idx, const int v) 
{
    assert (_is_setup);
    assert (idx >= 0 && idx < _field_cnt);
    assert (_pvalues[idx].is_setup());
    _pvalues[idx].set_int_value(v);
}

inline void table_row_t::set_value(const int idx, const bool v) 
{
    assert (_is_setup);
    assert (idx >= 0 && idx < _field_cnt);
    assert (_pvalues[idx].is_setup());
    _pvalues[idx].set_bit_value(v);
}

inline void table_row_t::set_value(const int idx, const short v) 
{
    assert (_is_setup);
    assert (idx >= 0 && idx < _field_cnt);
    assert (_pvalues[idx].is_setup());
    _pvalues[idx].set_smallint_value(v);
}

inline void table_row_t::set_value(const int idx, const double v) 
{
    assert (_is_setup);
    assert (idx >= 0 && idx < _field_cnt);
    assert (_pvalues[idx].is_setup());
    _pvalues[idx].set_float_value(v);
}

inline void table_row_t::set_value(const int idx, const decimal v) 
{
    assert (_is_setup);
    assert (idx >= 0 && idx < _field_cnt);
    assert (_pvalues[idx].is_setup());
    _pvalues[idx].set_decimal_value(v);
}

inline void table_row_t::set_value(const int idx, const time_t v) 
{
    assert (_is_setup);
    assert (idx >= 0 && idx < _field_cnt);
    assert (_is_setup && _pvalues[idx].is_setup());
    _pvalues[idx].set_time_value(v);
}

inline void table_row_t::set_value(const int idx, const char* string) 
{
    assert (_is_setup);
    assert (idx >= 0 && idx < _field_cnt);
    assert (_pvalues[idx].is_setup());

    register sqltype_t sqlt = _pvalues[idx].field_desc()->type();
    assert (sqlt == SQL_VARCHAR || sqlt == SQL_CHAR );

    int len = strlen(string);
    if ( sqlt == SQL_VARCHAR ) { 
        // if variable length
        _pvalues[idx].set_var_string_value(string, len);
    }
    else {
        // if fixed length
        _pvalues[idx].set_fixed_string_value(string, len);
    }
}

inline void table_row_t::set_value(const int idx, const timestamp_t& time)
{
    assert (_is_setup);
    assert (idx >= 0 && idx < _field_cnt);
    assert (_pvalues[idx].is_setup());
    _pvalues[idx].set_value(&time, 0);
}



/******************************************************************
 * 
 * GET value functions
 *
 ******************************************************************/

inline bool table_row_t::get_value(const int index,
                                   int& value) const
{
    assert (_is_setup);
    assert(index >= 0 && index < _field_cnt);
    if (_pvalues[index].is_null()) {
        value = 0;
        return false;
    }
    value = _pvalues[index].get_int_value();
    return true;
}

inline bool table_row_t::get_value(const int index,
                                   bool& value) const
{
    assert (_is_setup);
    assert(index >= 0 && index < _field_cnt);
    if (_pvalues[index].is_null()) {
        value = false;
        return false;
    }
    value = _pvalues[index].get_bit_value();
    return true;
}

inline bool table_row_t::get_value(const int index,
                                   short& value) const
{
    assert (_is_setup);
    assert(index >= 0 && index < _field_cnt);
    if (_pvalues[index].is_null()) {
        value = 0;
        return false;
    }
    value = _pvalues[index].get_smallint_value();
    return true;
}

inline bool table_row_t::get_value(const int index,
                                    char* buffer,
                                    const int bufsize) const
{
    assert (_is_setup);
    assert(index >= 0 && index < _field_cnt);
    if (_pvalues[index].is_null()) {
        buffer[0] = '\0';
        return (false);
    }
    // if variable length
    int sz = MIN(bufsize-1, _pvalues[index]._max_size);
    _pvalues[index].get_string_value(buffer, sz);
    buffer[sz] ='\0';
    return (true);
}

inline bool table_row_t::get_value(const int index,
                                   double& value) const
{
    assert (_is_setup);
    assert(index >= 0 && index < _field_cnt);
    if (_pvalues[index].is_null()) {
        value = 0;
        return false;
    }
    value = _pvalues[index].get_float_value();
    return true;
}

inline bool table_row_t::get_value(const int index,
                                   decimal& value) const
{
    assert (_is_setup);
    assert(index >= 0 && index < _field_cnt);
    if (_pvalues[index].is_null()) {
        value = decimal(0);
        return false;        
    }
    value = _pvalues[index].get_decimal_value();
    return true;
}

inline bool table_row_t::get_value(const int index,
                                   time_t& value) const
{
    assert (_is_setup);
    assert(index >= 0 && index < _field_cnt);
    if (_pvalues[index].is_null()) {
        return false;
    }
    value = _pvalues[index].get_time_value();
    return true;
}

inline bool table_row_t::get_value(const int index,
                                    timestamp_t& value) const
{
    assert (_is_setup);
    assert(index >= 0 && index < _field_cnt);
    if (_pvalues[index].is_null()) {
        return false;
    }
    value = _pvalues[index].get_tstamp_value();
    return true;
}



EXIT_NAMESPACE(shore);

#endif /* __SHORE_ROW_H */
