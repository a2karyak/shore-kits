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

/** @file:   shore_table_man.h
 *
 *  @brief:  Base classes for tables and iterators over tables and indexes
 *           stored in Shore.
 *
 *  @note:   table_man_impl       - class for table-related operations
 *           table_scan_iter_impl - table scanner
 *           index_scan_iter_impl - index scanner
 *
 *  @author: Ippokratis Pandis, January 2008
 *
 */

/* shore_table_man.h contains the template-based base classes (table_man_impl) 
 * for operations on tables stored in Shore. It also contains the template-based
 * classes (table_scan_iter_impl) and (index_scan_iter_impl) for iteration 
 * operations.
 *
 *
 * FUNCTIONALITY 
 *
 * Operations on single tuples, including adding, updating, and index probe are
 * provided as well, as part of either the (table_man_impl) class.
 */

#ifndef __SHORE_TABLE_MANAGER_H
#define __SHORE_TABLE_MANAGER_H


#include "sm_vas.h"
#include "util.h"
#include "atomic_trash_stack.h"

#include "shore_table.h"
#include "shore_row_impl.h"
#include "shore_row_cache.h"


ENTER_NAMESPACE(shore);




/* ---------------------------------------------------------------
 *
 * @brief: Macros for correct offset calculation
 *
 * --------------------------------------------------------------- */

#define VAR_SLOT(start, offset)   ((offset_t*)((start)+(offset)))
#define SET_NULL_FLAG(start, offset)                            \
    (*(char*)((start)+((offset)>>3))) &= (1<<((offset)>>3))
#define IS_NULL_FLAG(start, offset)                     \
    (*(char*)((start)+((offset)>>3)))&(1<<((offset)>>3))




/* ---------------------------------------------------------------
 *
 * @brief: Forward declarations
 *
 * --------------------------------------------------------------- */

template <class TableDesc>
class table_scan_iter_impl;

template <class TableDesc>
class index_scan_iter_impl;



/* ---------------------------------------------------------------
 *
 * @class: table_man_impl
 *
 * @brief: Template-based class that operates on a Shore table. 
 *
 * --------------------------------------------------------------- */

template <class TableDesc>
class table_man_impl : public table_man_t
{
public:
    typedef row_impl<TableDesc> table_tuple; 
    typedef table_scan_iter_impl<TableDesc> table_iter;
    typedef index_scan_iter_impl<TableDesc> index_iter;
    typedef row_cache_t<TableDesc> row_cache;

protected:

    TableDesc* _ptable;      /* pointer back to the table description */
    guard<row_cache> _pcache;      /* pointer to a tuple cache */
    guard<ats_char_t> _pts;  /* trash stack */


public:

    /* ------------------- */
    /* --- Constructor --- */
    /* ------------------- */

    table_man_impl(TableDesc* aTableDesc, 
                   int row_count=DEFAULT_INIT_ROW_COUNT,
                   bool construct_cache=true)
        : _ptable(aTableDesc)
    {
        assert (_ptable);
        assert (row_count>=0);

        // init tuple cache
        if (construct_cache) {
            _pcache = new row_cache(_ptable, row_count);

            // init trash stack            
            _pts = new ats_char_t(_ptable->maxsize());
        }
    }
    
    /* ----------------------------- */
    /* --- formatting operations --- */
    /* ----------------------------- */

    /* format tuple */
    const int  format(table_tuple* ptuple, 
                      rep_row_t &arep);

    /* load tuple from input buffer */
    const bool load(table_tuple* ptuple,
                    const char* string);

    /* disk space needed for tuple */
    const int  size(table_tuple* ptuple) const; 

    /* format the key value */
    const int  format_key(index_desc_t* pindex, 
                          table_tuple* ptuple, 
                          rep_row_t &arep);

    /* load key index from input buffer */
    const bool load_key(const char* string,
                        index_desc_t* pindex,
                        table_tuple* ptuple);

    /* set indexed fields of the row to minimum */
    const int  min_key(index_desc_t* pindex, 
                       table_tuple* ptuple, 
                       rep_row_t &arep);

    /* set indexed fields of the row to maximum */
    const int  max_key(index_desc_t* pindex, 
                       table_tuple* ptuple, 
                       rep_row_t &arep);

    /* length of the formatted key */
    const int  key_size(index_desc_t* pindex, 
                        const table_tuple* ptuple) const;


    int get_pnum(index_desc_t* pindex, table_tuple const* ptuple) const;

    /* ---------------------------- */
    /* --- access through index --- */
    /* ---------------------------- */

    /* idx probe */
    inline w_rc_t   index_probe(ss_m* db,
                                index_desc_t* pidx,
                                table_tuple*  ptuple,
                                lock_mode_t   lock_mode = SH,          /* one of: NL, SH, EX */
                                latch_mode_t  latch_mode = LATCH_SH);  /* one of: LATCH_SH, LATCH_EX */
    
    /* probe idx in EX (& LATCH_EX) mode */
    inline w_rc_t   index_probe_forupdate(ss_m* db,
                                   index_desc_t* pidx,
                                   table_tuple*  ptuple)
    {
        return (index_probe(db, pidx, ptuple, EX, LATCH_EX));
    }

    /* probe idx in NL (& LATCH_SH) mode */
    inline w_rc_t   index_probe_nl(ss_m* db,
                                   index_desc_t* pidx,
                                   table_tuple*  ptuple)
    {
        return (index_probe(db, pidx, ptuple, NL, LATCH_SH));
    }

    /* probe primary idx */
    inline w_rc_t   index_probe_primary(ss_m* db, 
                                        table_tuple* ptuple, 
                                        lock_mode_t  lock_mode = SH,        /* one of: NL, SH, EX */
                                        latch_mode_t latch_mode = LATCH_SH) /* one of: LATCH_SH, LATCH_EX */
    {
        return (index_probe(db, _primary_idx, ptuple, lock_mode, latch_mode));
    }


    // by name probes


    // idx probe - based on idx name //
    inline w_rc_t   index_probe_by_name(ss_m* db, 
                                        const char*  idx_name, 
                                        table_tuple* ptuple,
                                        lock_mode_t  lock_mode = SH,        /* one of: NL, SH, EX */
                                        latch_mode_t latch_mode = LATCH_SH) /* one of: LATCH_SH, LATCH_EX */
    {
        index_desc_t* pindex = _ptable->find_index(idx_name);
        return (index_probe(db, pindex, ptuple, lock_mode, latch_mode));
    }

    // probe idx in EX (& LATCH_EX) mode - based on idx name //
    inline w_rc_t   index_probe_forupdate_by_name(ss_m* db, 
                                                  const char* idx_name,
                                                  table_tuple* ptuple) 
    { 
	index_desc_t* pindex = _ptable->find_index(idx_name);
	return (index_probe_forupdate(db, pindex, ptuple));
    }

    // probe idx in NL (& LATCH_NL) mode - based on idx name //
    inline w_rc_t   index_probe_nl_by_name(ss_m* db, 
                                           const char* idx_name,
                                           table_tuple* ptuple) 
    { 
	index_desc_t* pindex = _ptable->find_index(idx_name);
	return (index_probe_nl(db, pindex, ptuple));
    }


    /* -------------------------- */
    /* --- tuple manipulation --- */
    /* -------------------------- */

    w_rc_t    add_tuple(ss_m* db, table_tuple* ptuple, lock_mode_t lm = EX);
    w_rc_t    update_tuple(ss_m* db, table_tuple* ptuple, lock_mode_t lm = EX);
    w_rc_t    delete_tuple(ss_m* db, table_tuple* ptuple, lock_mode_t lm = EX);


    /* ------------------------------------------- */
    /* --- iterators for index and table scans --- */
    /* ------------------------------------------- */

    w_rc_t get_iter_for_file_scan(ss_m* db,
				  table_iter* &iter,
                                  lock_mode_t alm = SH);

    w_rc_t get_iter_for_index_scan(ss_m* db,
				   index_desc_t* pindex,
				   index_iter* &iter,
                                   lock_mode_t alm,
                                   bool need_tuple,
				   scan_index_i::cmp_t c1,
				   const cvec_t & bound1,
				   scan_index_i::cmp_t c2,
				   const cvec_t & bound2);


    /* ------------------------------------------------------- */
    /* --- check consistency between the indexes and table --- */
    /* ------------------------------------------------------- */

    /**  true:  consistent
     *   false: inconsistent */
    w_rc_t check_all_indexes_together(ss_m* db);
    bool   check_all_indexes(ss_m* db);
    w_rc_t check_index(ss_m* db, index_desc_t* pidx);
    w_rc_t scan_all_indexes(ss_m* db);
    w_rc_t scan_index(ss_m* db, index_desc_t* pidx);


    /* ----------------- */
    /* --- debugging --- */
    /* ----------------- */

    w_rc_t print_table(ss_m* db); /* print the table on screen */


    /* ------------------------------ */
    /* --- trash stack operations --- */
    /* ------------------------------ */

    ats_char_t* ts() { assert (_pts); return (_pts); }


    /* ------------------------------ */
    /* --- tuple cache operations --- */
    /* ------------------------------ */

    row_cache* get_cache() { assert (_pcache); return (_pcache); }

    inline table_tuple* get_tuple() 
    {
        //assert (_pcache);
        return (_pcache->borrow());
    }
    

    inline void give_tuple(table_tuple* ptt) 
    {
        //assert (_pcache);
        _pcache->giveback(ptt);
    }

    void print_cache_stats() 
    {
        assert (_pcache);
        TRACE( TRACE_STATISTICS, "(%s) tuple cache statistics\n", 
               _ptable->name());
        //_pcache->print_stats();
    }

}; // EOF: table_man_impl



/* ---------------------------------------------------------------
 *
 * @class: table_scan_iter_impl
 *
 * @brief: Declaration of a table (file) scan iterator
 *
 * --------------------------------------------------------------- */

template <class TableDesc>
class table_scan_iter_impl : 
    public tuple_iter_t<TableDesc, scan_file_i, row_impl<TableDesc> >
{
public:
    typedef row_impl<TableDesc> table_tuple;
    typedef table_man_impl<TableDesc> table_manager;

private:

    table_manager* _pmanager;

public:

    /* -------------------- */
    /* --- construction --- */
    /* -------------------- */

    table_scan_iter_impl(ss_m* db, 
                         TableDesc* ptable,
                         table_manager* pmanager,
                         lock_mode_t alm) 
        : tuple_iter_t(db, ptable, alm, true), _pmanager(pmanager)
    { 
        assert (_pmanager);
        W_COERCE(open_scan(_db)); 
    }
        
    ~table_scan_iter_impl() { close_scan(); }


    /* ------------------------ */
    /* --- fscan operations --- */
    /* ------------------------ */

    w_rc_t open_scan(ss_m* db) {
        if (!_opened) {
            W_DO(_file->check_fid(db));
            _scan = new scan_file_i(_file->fid(), ss_m::t_cc_record, false, _lm);
            _opened = true;
        }
        return (RCOK);
    }


    pin_i* cursor() {
        pin_i *rval;
        bool eof;
        _scan->cursor(rval, eof);
        return (eof? NULL : rval);
    }


    w_rc_t next(ss_m* db, bool& eof, table_tuple& tuple) {
        assert (_pmanager);
        if (!_opened) open_scan(db);
        pin_i* handle;
        W_DO(_scan->next(handle, 0, eof));
        if (!eof) {
            if (!_pmanager->load(&tuple, handle->body()))
                return RC(se_WRONG_DISK_DATA);
            tuple.set_rid(handle->rid());
        }
        return (RCOK);
    }

}; // EOF: table_scan_iter_impl



/* ---------------------------------------------------------------------
 *
 * @class: index_scan_iter_impl
 *
 * @brief: Declaration of a index scan iterator
 *
 * --------------------------------------------------------------------- */


template <class TableDesc>
class index_scan_iter_impl : 
    public tuple_iter_t<index_desc_t, scan_index_i, row_impl<TableDesc> >
{
public:
    typedef row_impl<TableDesc> table_tuple;
    typedef table_man_impl<TableDesc> table_manager;

private:
    table_manager* _pmanager;
    bool           _need_tuple;

public:

    /* -------------------- */
    /* --- construction --- */
    /* -------------------- */
        
    index_scan_iter_impl(ss_m* db,
                         index_desc_t* pindex,
                         table_manager* pmanager,
                         lock_mode_t alm,    // alm = SH
                         bool need_tuple)    // need_tuple = false
        : tuple_iter_t(db, pindex, alm, true), 
          _pmanager(pmanager), _need_tuple(need_tuple)
    { 
        assert (_pmanager);
        /** @note: We need to know the bounds of the iscan before
         *         opening the iterator. That's why we cannot open
         *         the iterator upon construction.
         *         Needs explicit call to open_scan(...)
         */        
    }

    /** In case we know the bounds of the iscan a-priori */
    index_scan_iter_impl(ss_m* db,
                         index_desc_t* pindex,
                         table_manager* pmanager,
                         lock_mode_t alm,
                         bool need_tuple,
                         scan_index_i::cmp_t cmp1, const cvec_t& bound1,
                         scan_index_i::cmp_t cmp2, const cvec_t& bound2,
                         const int maxkeysize) 
        : tuple_iter_t(db, pindex, alm, true), 
          _pmanager(pmanager), _need_tuple(need_tuple)
    { 
        assert (_pmanager);
        W_COERCE(open_scan(db, cmp1, bound1, cmp2, bound2, maxkeysize));
    }
        
    ~index_scan_iter_impl() { close_scan(); };


    /* ------------------------ */        
    /* --- iscan operations --- */
    /* ------------------------ */

    w_rc_t open_scan(ss_m* db, int pnum,
                     scan_index_i::cmp_t c1, const cvec_t& bound1,
                     scan_index_i::cmp_t c2, const cvec_t& bound2,
                     const int maxkeysize)
    {                    
        if (!_opened) {

            // 1. figure out what concurrency will be used
            // !! according to shore-mt/src/scan.h:82 
            //    t_cc_kvl  - IS lock on the index and SH key-value locks on every entry encountered
            //    t_cc_none - IS lock on the index and no other locks
            ss_m::concurrency_t cc = ss_m::t_cc_kvl;
            if (_lm==NL) cc = ss_m::t_cc_none;

            // 2. open the cursor
            W_DO(_file->check_fid(db));
            _scan = new scan_index_i(_file->fid(pnum), c1, bound1, c2, bound2,
                                     false, cc, _lm);
            _opened = true;
        }

        return (RCOK);
    }

    w_rc_t next(ss_m* db, bool& eof, table_tuple& tuple) 
    {
        assert (_opened);
        assert (_pmanager);
        assert (tuple._rep);

        W_DO(_scan->next(eof));

        if (!eof) {
            int key_sz = _pmanager->format_key(_file, &tuple, *tuple._rep);
            assert (tuple._rep->_dest); // if dest == NULL there is an invalid key

            vec_t    key(tuple._rep->_dest, key_sz);

            rid_t    rid;
            vec_t    record(&rid, sizeof(rid_t));
            smsize_t klen = 0;
            smsize_t elen = sizeof(rid_t);

            W_DO(_scan->curr(&key, klen, &record, elen));
            tuple.set_rid(rid);
            
            _pmanager->load_key((const char*)key.ptr(0), _file, &tuple);
            //tuple.load_key(key.ptr(0), _file);

            if (_need_tuple) {
                pin_i  pin;
                W_DO(pin.pin(rid, 0, _lm));
                if (!_pmanager->load(&tuple, pin.body()))
                    return RC(se_WRONG_DISK_DATA);
                pin.unpin();
            }
        }    
        return (RCOK);
    }

}; // EOF: index_scan_iter_impl




/********************************************************************* 
 *
 *  table_man_impl methods
 *
 *********************************************************************/ 



/* ---------------------------- */
/* --- formating operations --- */
/* ---------------------------- */


/********************************************************************* 
 *
 *  @fn:      format
 *
 *  @brief:   Return a string of the tuple (array of pvalues[]) formatted 
 *            to the appropriate disk format so it can be pushed down to 
 *            data pages in Shore. The size of the data buffer is in 
 *            parameter (bufsz).
 *
 *  @warning: This function should be the inverse of the load() 
 *            function changes to one of the two functions should be
 *            mirrored to the other.
 *
 *  @note:    convert: memory -> disk format
 *
 *********************************************************************/

template <class TableDesc>
const int table_man_impl<TableDesc>::format(table_tuple* ptuple,
                                            rep_row_t &arep)
{
    // Format the data field by field


    // 1. Get the pre-calculated offsets

    // current offset for fixed length field values
    register offset_t fixed_offset = ptuple->get_fixed_offset();

    // current offset for variable length field slots
    register offset_t var_slot_offset = ptuple->get_var_slot_offset();

    // current offset for variable length field values
    register offset_t var_offset = ptuple->get_var_offset();

    

    // 2. calculate the total space of the tuple
    //   (tupsize)    : total space of the tuple 

    register int tupsize    = 0;

    // look at shore_row_impl.h:100
    register int null_count = ptuple->get_null_count();
    register int fixed_size = ptuple->get_var_slot_offset() - ptuple->get_fixed_offset();


    // loop over all the varialbe-sized fields and add their real size (set at ::set())
    for (int i=0; i<_ptable->field_count(); i++) {

	if (ptuple->_pvalues[i].is_variable_length()) {
            // If it is of VARIABLE length, then if the value is null
            // do nothing, else add to the total tuple length the (real)
            // size of the value plus the size of an offset.

            if (ptuple->_pvalues[i].is_null()) continue;
            tupsize += ptuple->_pvalues[i].realsize();
            tupsize += sizeof(offset_t);
	}

        // If it is of FIXED length, then increase the total tuple
        // length, as well as, the size of the fixed length part of
        // the tuple by the fixed size of this type of field.

        // IP: The length of the fixed-sized fields is added after the loop
    }

    // Add up the length of the fixed-sized fields
    tupsize += fixed_size;

    // In the total tuple length add the size of the bitmap that
    // shows which fields can be NULL 
    if (null_count) tupsize += (null_count >> 3) + 1;
    assert (tupsize);



    // 3. allocate space for the formatted data
    arep.set(tupsize);



    // 4. Copy the fields to the array, field by field

    register int null_index = -1;
    // iterate over all fields
    for (int i=0; i<_ptable->field_count(); i++) {

        // Check if the field can be NULL. 
        // If it can be NULL, increase the null_index, and 
        // if it is indeed NULL set the corresponding bit
	if (ptuple->_pvalues[i].field_desc()->allow_null()) {
	    null_index++;
	    if (ptuple->_pvalues[i].is_null()) {
		SET_NULL_FLAG(arep._dest, null_index);
	    }
	}

        // Check if the field is of VARIABLE length. 
        // If it is, copy the field value to the variable length part of the
        // buffer, to position  (buffer + var_offset)
        // and increase the var_offset.
	if (ptuple->_pvalues[i].is_variable_length()) {
	    ptuple->_pvalues[i].copy_value(arep._dest + var_offset);
            register int offset = ptuple->_pvalues[i].realsize(); 
	    var_offset += offset;

	    // set the offset 
            offset_t len = offset;
	    memcpy(VAR_SLOT(arep._dest, var_slot_offset), &len, sizeof(offset_t));
	    var_slot_offset += sizeof(offset_t);
	}
	else {
            // If it is of FIXED length, then copy the field value to the
            // fixed length part of the buffer, to position 
            // (buffer + fixed_offset)
            // and increase the fixed_offset
	    ptuple->_pvalues[i].copy_value(arep._dest + fixed_offset);
	    fixed_offset += ptuple->_pvalues[i].maxsize();
	}
    }
    return (tupsize);
}


/*********************************************************************
 *
 *  @fn:      load
 *
 *  @brief:   Given a tuple in disk format, read it back into memory
 *            (_pvalues[] array).
 *
 *  @warning: This function should be the inverse of the format() function
 *            changes to one of the two functions should be mirrored to
 *            the other.
 *
 *  @note:    convert: disk -> memory format
 *
 *********************************************************************/

template <class TableDesc>
const bool table_man_impl<TableDesc>::load(table_tuple* ptuple,
                                           const char* data)
{
    // Read the data field by field


    // 1. Get the pre-calculated offsets

    // current offset for fixed length field values
    register offset_t fixed_offset = ptuple->get_fixed_offset();

    // current offset for variable length field slots
    register offset_t var_slot_offset = ptuple->get_var_slot_offset();

    // current offset for variable length field values
    register offset_t var_offset = ptuple->get_var_offset();



    // 2. Read the data field by field

    int null_index = -1;
    for (int i=0; i<_ptable->field_count(); i++) {

        // Check if the field can be NULL.
        // If it can be NULL, increase the null_index,
        // and check if the bit in the null_flags bitmap is set.
        // If it is set, set the corresponding value in the tuple 
        // as null, and go to the next field, ignoring the rest
	if (ptuple->_pvalues[i].field_desc()->allow_null()) {
	    null_index++;
	    if (IS_NULL_FLAG(data, null_index)) {
		ptuple->_pvalues[i].set_null();
		continue;
	    }
	}

        // Check if the field is of VARIABLE length.
        // If it is, copy the offset of the value from the offset part of the
        // buffer (pointed by var_slot_offset). Then, copy that many chars from
        // the variable length part of the buffer (pointed by var_offset). 
        // Then increase by one offset index, and offset of the pointer of the 
        // next variable value
	if (ptuple->_pvalues[i].is_variable_length()) {
	    offset_t var_len;
	    memcpy(&var_len,  VAR_SLOT(data, var_slot_offset), sizeof(offset_t));
	    ptuple->_pvalues[i].set_value(data+var_offset, var_len);
	    var_offset += var_len;
	    var_slot_offset += sizeof(offset_t);
	}
	else {
            // If it is of FIXED length, copy the data from the fixed length
            // part of the buffer (pointed by fixed_offset), and the increase
            // the fixed offset by the (fixed) size of the field
	    ptuple->_pvalues[i].set_value(data+fixed_offset, 
                                          ptuple->_pvalues[i].maxsize());
	    fixed_offset += ptuple->_pvalues[i].maxsize();
	}
    }
    return (true);
}


/****************************************************************** 
 *
 *  @fn:      format_key
 *
 *  @brief:   Gets an index and for a selected row it copies to the
 *            passed buffer only the fields that are contained in the 
 *            index and returns the size of the newly allocated buffer, 
 *            which is the key_size for the index. The size of the 
 *            data buffer is in parameter (bufsz).
 *
 *  @warning: This function should be the inverse of the load_key() 
 *            function changes to one of the two functions should be
 *            mirrored to the other.
 *
 *  @note:    !!! Uses the maxsize() of each field, so even the
 *            variable legnth fields will be treated as of fixed size
 *
 ******************************************************************/

template <class TableDesc>
inline const int table_man_impl<TableDesc>::format_key(index_desc_t* pindex,
                                                       table_tuple* ptuple,
                                                       rep_row_t &arep)
{
    assert (_ptable);
    assert (pindex);
    assert (ptuple);

    /* 1. calculate the key size */
    int isz = key_size(pindex, ptuple);
    assert (isz);

    
    /* 2. allocate buffer space, if necessary */
    arep.set(isz);


    /* 3. write the buffer */
    register offset_t offset = 0;
    for (int i=0; i<pindex->field_count(); i++) {
        register int ix = pindex->key_index(i);
        register field_value_t* pfv = &ptuple->_pvalues[ix];

        // copy value
        if (!pfv->copy_value(arep._dest+offset)) {
            assert (false); // problem in copying value
            return (0);
        }
        
        // IP: previously it was making distinction whether
        // the field was of fixed or variable length
        offset += pfv->maxsize();
    }
    return (isz);
}



/*********************************************************************
 *
 *  @fn:      load_key
 *
 *  @brief:   Given a buffer with the representation of the tuple in 
 *            disk format, read back into memory (to _pvalues[] array),
 *            but it reads only the fields that are contained to the
 *            specified index.
 *
 *  @warning: This function should be the inverse of the format_key() 
 *            function changes to one of the two functions should be
 *            mirrored to the other.
 *
 *  @note:    convert: disk -> memory format (for the key)
 *
 *********************************************************************/

template <class TableDesc>
const bool table_man_impl<TableDesc>::load_key(const char* string,
                                               index_desc_t* pindex,
                                               table_tuple* ptuple)
{
    assert (_ptable);
    assert (pindex);
    assert (string);

    register int offset = 0;
    for (int i=0; i<pindex->field_count(); i++) {
	register int field_index = pindex->key_index(i);
        register int size = ptuple->_pvalues[field_index].maxsize();
        ptuple->_pvalues[field_index].set_value(string + offset, size);
        offset += size;
    }

    return (true);
}



/****************************************************************** 
 *
 *  @fn:    min_key/max_key
 *
 *  @brief: Gets an index and for a selected row it sets all the 
 *          fields that are contained in the index to their 
 *          minimum or maximum value
 *
 ******************************************************************/

template <class TableDesc>
inline const int table_man_impl<TableDesc>::min_key(index_desc_t* pindex, 
                                                    table_tuple* ptuple,
                                                    rep_row_t &arep)
{
    assert (_ptable);
    for (int i=0; i<pindex->field_count(); i++) {
	int field_index = pindex->key_index(i);
	ptuple->_pvalues[field_index].set_min_value();
    }
    return (format_key(pindex, ptuple, arep));
}


template <class TableDesc>
inline const int table_man_impl<TableDesc>::max_key(index_desc_t* pindex, 
                                                    table_tuple* ptuple,
                                                    rep_row_t &arep)
{
    assert (_ptable);
    for (int i=0; i<pindex->field_count(); i++) {
	int field_index = pindex->key_index(i);
	ptuple->_pvalues[field_index].set_max_value();
    }
    return (format_key(pindex, ptuple, arep));
}



/****************************************************************** 
 *
 *  @fn:    key_size
 *
 *  @brief: For an index and a selected row it returns the 
 *          real or maximum size of the index key
 *
 *  @note: !!! Uses the maxsize() of each field, so even the
 *         variable legnth fields will be treated as of fixed size
 *
 *  @note: Since all fields of an index are of fixed length
 *         key_size() == maxkeysize()
 *
 ******************************************************************/

template <class TableDesc>
inline const int table_man_impl<TableDesc>::key_size(index_desc_t* pindex, 
                                                     const table_tuple*) const
{
    assert (_ptable);
    return (_ptable->index_maxkeysize(pindex));

#if 0
    // !! Old implementation, which uses not the maximum size
    //    for the VARCHARs.
    int size = 0;
    register int ix = 0;
    for (int i=0; i<index->field_count(); i++) {
        ix = index->key_index(i);
        size += prow->_pvalues[ix].maxsize();
    }
    return (size);
#endif
}



/* ---------------------------- */
/* --- access through index --- */
/* ---------------------------- */


/********************************************************************* 
 *
 *  @fn:    index_probe
 *  
 *  @brief: Finds the rid of the specified key using a certain index
 *
 *  @note:  The key is parsed from the tuple that it is passed as parameter
 *
 *********************************************************************/

template <class TableDesc>
w_rc_t table_man_impl<TableDesc>::index_probe(ss_m* db,
                                              index_desc_t* pindex,
                                              table_tuple*  ptuple,
                                              lock_mode_t   lock_mode,
                                              latch_mode_t  latch_mode)
{
    assert (_ptable);
    assert (pindex);
    assert (ptuple); 
    assert (ptuple->_rep);

    bool     found = false;
    smsize_t len = sizeof(rid_t);

#ifdef CFG_DORA
    // 0. if index created with NO-LOCK option (DORA) then:
    //    - ignore lock mode (use NL)
    //    - find_assoc ignoring any locks
    bool bIgnoreLocks = false;
    if (pindex->is_relaxed()) {
        lock_mode   = NL;
        bIgnoreLocks = true;
    }
#endif

    // 1. ensure valid index
    W_DO(pindex->check_fid(db));

    // 2. find the tuple in the index
    int key_sz = format_key(pindex, ptuple, *ptuple->_rep);
    assert (ptuple->_rep->_dest); // if NULL invalid key

    int pnum = get_pnum(pindex, ptuple);

    W_DO(ss_m::find_assoc(pindex->fid(pnum),
    			  vec_t(ptuple->_rep->_dest, key_sz),
    			  &(ptuple->_rid),
    			  len,
    			  found
#ifdef CFG_DORA
                          ,bIgnoreLocks
#endif
                          ));
    
    if (!found) return RC(se_TUPLE_NOT_FOUND);

    // 3. read the tuple
    pin_i pin;
    W_DO(pin.pin(ptuple->rid(), 0, lock_mode, latch_mode));

    if (!load(ptuple, pin.body())) return RC(se_WRONG_DISK_DATA);
    pin.unpin();
  
    return (RCOK);
}


template <class TableDesc>
int table_man_impl<TableDesc>::get_pnum(index_desc_t* pindex, table_tuple const* ptuple) const {
    assert(ptuple);
    assert(pindex);
    if(!pindex->is_partitioned())
	return 0;

    int first_key;
    ptuple->get_value(pindex->key_index(0), first_key);
    return first_key % pindex->get_partition_count();
}





/* -------------------------- */
/* --- tuple manipulation --- */
/* -------------------------- */



/********************************************************************* 
 *
 *  @fn:    add_tuple
 *
 *  @brief: Inserts a tuple to a table and all the indexes of the table
 *
 *  @note:  This function should be called in the context of a trx.
 *          The passed tuple should be formed. If everything goes as
 *          expected the _rid of the tuple will be set. 
 *
 *********************************************************************/

template <class TableDesc>
w_rc_t table_man_impl<TableDesc>::add_tuple(ss_m* db, 
                                            table_tuple* ptuple,
                                            lock_mode_t lm)
{
    assert (_ptable);
    assert (ptuple);
    assert (ptuple->_rep);

    // 1. find the file
    W_DO(_ptable->check_fid(db));

#ifdef CFG_DORA
    // 2. figure out what mode will be used
    bool bIgnoreLocks = false;
    if (lm==NL) bIgnoreLocks = true;
#endif

    // 3. append the tuple
    int tsz = format(ptuple, *ptuple->_rep);
    assert (ptuple->_rep->_dest); // if NULL invalid

    W_DO(db->create_rec(_ptable->fid(), vec_t(), tsz,
                        vec_t(ptuple->_rep->_dest, tsz),
                        ptuple->_rid,
                        serial_t::null
#ifdef CFG_DORA
                        ,bIgnoreLocks
#endif
                        ));

    // 4. update the indexes
    index_desc_t* index = _ptable->indexes();
    int ksz = 0;

    while (index) {
        ksz = format_key(index, ptuple, *ptuple->_rep);
        assert (ptuple->_rep->_dest); // if dest == NULL there is invalid key

	int pnum = get_pnum(index, ptuple);
        W_DO(index->find_fid(db, pnum));
	W_DO(db->create_assoc(index->fid(pnum),
                              vec_t(ptuple->_rep->_dest, ksz),
                              vec_t(&(ptuple->_rid), sizeof(rid_t))
#ifdef CFG_DORA
                              ,bIgnoreLocks
#endif
                              ));

        // move to next index
	index = index->next();
    }
    return (RCOK);
}



/********************************************************************* 
 *
 *  @fn:    update_tuple
 *
 *  @brief: Updates a tuple from a table
 *
 *  @note:  This function should be called in the context of a trx.
 *          The passed tuple should be valid.
 *          There is no need of updating the indexes.
 *
 *  !!! In order to update a field included by an index !!!
 *  !!! the tuple should be deleted and inserted again  !!!
 *
 *********************************************************************/

template <class TableDesc>
w_rc_t table_man_impl<TableDesc>::update_tuple(ss_m* db, 
                                               table_tuple* ptuple,
                                               lock_mode_t  lm)
{
    assert (_ptable);
    assert (ptuple);
    assert (ptuple->_rep);

    if (!ptuple->is_rid_valid()) return RC(se_NO_CURRENT_TUPLE);

    // 0. figure out what mode will be used
    latch_mode_t pin_latch_mode = LATCH_EX;

#ifdef CFG_DORA
    bool bIgnoreLocks = false;
    if (lm==NL) {
        //        pin_latch_mode = LATCH_SH;
        bIgnoreLocks = true;
    }
#endif

    // 1. pin record
    pin_i pin;
    W_DO(pin.pin(ptuple->rid(), 0, lm, pin_latch_mode));
    int current_size = pin.body_size();

    // 2. update record
    int tsz = format(ptuple, *ptuple->_rep);
    assert (ptuple->_rep->_dest); // if NULL invalid

    // 2a. if updated record cannot fit in the previous spot
    w_rc_t rc;
    if (current_size < tsz) {
        rc = pin.append_rec(zvec_t(tsz - current_size));

        // on error unpin 
        if (rc.is_error()) {
            TRACE( TRACE_DEBUG, "Error updating (by append) record\n");
            pin.unpin();
        }
        W_DO(rc);
    }

    // 2b. else, simply update
    rc = pin.update_rec(0, vec_t(ptuple->_rep->_dest, tsz), 0
#ifdef CFG_DORA
                        ,bIgnoreLocks
#endif
                        );

    if (rc.is_error()) TRACE( TRACE_DEBUG, "Error updating record\n");

    // 3. unpin
    pin.unpin();
    return (rc);
}



/********************************************************************* 
 *
 *  @fn:    delete_tuple
 *
 *  @brief: Deletes a tuple from a table and the corresponding entries
 *          on all the indexes of the table
 *
 *  @note:  This function should be called in the context of a trx
 *          The passed tuple should be valid.
 *
 *********************************************************************/

template <class TableDesc>
w_rc_t table_man_impl<TableDesc>::delete_tuple(ss_m* db, 
                                               table_tuple* ptuple,
                                               lock_mode_t lm)
{
    assert (_ptable);
    assert (ptuple);
    assert (ptuple->_rep);

    if (!ptuple->is_rid_valid()) return RC(se_NO_CURRENT_TUPLE);

    rid_t todelete = ptuple->rid();

#ifdef CFG_DORA
    // 1. figure out what mode will be used
    bool bIgnoreLocks = false;
    if (lm==NL) bIgnoreLocks = true;
#endif

    // 2. delete all the corresponding index entries
    index_desc_t* pindex = _ptable->indexes();
    int key_sz = 0;

    while (pindex) {
        key_sz = format_key(pindex, ptuple, *ptuple->_rep);
        assert (ptuple->_rep->_dest); // if NULL invalid key

	int pnum = get_pnum(pindex, ptuple);
	W_DO(pindex->find_fid(db, pnum));
	W_DO(db->destroy_assoc(pindex->fid(pnum),
                               vec_t(ptuple->_rep->_dest, key_sz),
                               vec_t(&(todelete), sizeof(rid_t))
#ifdef CFG_DORA
                               ,bIgnoreLocks
#endif
                               ));

        // move to next index
	pindex = pindex->next();
    }

    // 3. delete the tuple
    W_DO(db->destroy_rec(todelete
#ifdef CFG_DORA
                         ,bIgnoreLocks
#endif
                         ));

    // invalidate tuple
    ptuple->set_rid(rid_t::null);
    return (RCOK);
}




/* ------------------------------------------- */
/* --- iterators for index and table scans --- */
/* ------------------------------------------- */


/********************************************************************* 
 *
 *  @fn:    get_iter_for_scan (table/index)
 *  
 *  @brief: Returns and opens an (table/index) scan iterator.
 *
 *  @note:  If it fails to open the iterator it retuns an error. 
 *
 *********************************************************************/

template <class TableDesc>
w_rc_t table_man_impl<TableDesc>::get_iter_for_file_scan(ss_m* db,
                                                         table_iter* &iter,
                                                         lock_mode_t alm)
{
    assert (_ptable);
    iter = new table_scan_iter_impl<TableDesc>(db, _ptable, this, alm);
    if (iter->opened()) return (RCOK);
    return RC(se_OPEN_SCAN_ERROR);
}


template <class TableDesc>
w_rc_t table_man_impl<TableDesc>::get_iter_for_index_scan(ss_m* db,
                                                          index_desc_t* index,
                                                          index_iter* &iter,
                                                          lock_mode_t alm,
                                                          bool need_tuple,
                                                          scan_index_i::cmp_t c1,
                                                          const cvec_t& bound1,
                                                          scan_index_i::cmp_t c2,
                                                          const cvec_t& bound2)
{
    assert (_ptable);
    int pnum = 0;
    if(index->is_partitioned()) {
	int key0 = 0;
	int cnt = bound1.copy_to(&key0, sizeof(int));
	assert(cnt == sizeof(int));
	int other_key0;
	cnt = bound2.copy_to(&other_key0, sizeof(int));
	assert(cnt == sizeof(int));
	assert(key0 == other_key0);
	pnum = key0 % index->get_partition_count();
    }
    iter = new index_scan_iter_impl<TableDesc>(db, index, this, alm, need_tuple);
    W_DO(iter->open_scan(db, pnum, c1, bound1, c2, bound2, 
                         _ptable->index_maxkeysize(index)));
    if (iter->opened())  return (RCOK);
    return RC(se_OPEN_SCAN_ERROR);
}



#define CHECK_FOR_DEADLOCK(action, on_deadlock)				\
    do {								\
	w_rc_t rc = action;						\
	if(rc.is_error()) {						\
	    W_COERCE(db->abort_xct());					\
	    if(rc.err_num() == smlevel_0::eDEADLOCK) {			\
		TRACE( TRACE_ALWAYS, "load(%s): %d: deadlock detected. Retrying.n", _ptable->name(), tuple_count); \
		W_DO(db->begin_xct());					\
		on_deadlock;						\
	    }								\
	    W_DO(rc);							\
	}								\
	else {								\
	    break;							\
	}								\
    } while(1) 

struct table_creation_lock {
    static pthread_mutex_t* get_lock() {
	static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	return &lock;
    }
};



EXIT_NAMESPACE(shore);
#include "shore_helper_loader.h"
ENTER_NAMESPACE(shore);


/******************************************************************** 
 *
 *  @fn:    check_all_indexes_together
 *
 *  @brief: Check all indexes with a single file scan
 *
 *  @note:  Can be used for warm-up for memory-fitting databases
 *
 ********************************************************************/

template <class TableDesc>
w_rc_t table_man_impl<TableDesc>::check_all_indexes_together(ss_m* db)
{
    assert (_ptable);

    TRACE( TRACE_DEBUG, "Checking consistency of the indexes on table (%s)\n",
           _ptable->name());

    time_t tstart = time(NULL);
    
    W_DO(db->begin_xct());
    index_desc_t* pindex = NULL;

    // get a table iterator
    table_iter* iter;
    W_DO(get_iter_for_file_scan(db, iter));

    // scan the entire file    
    bool eof = false;
    table_tuple tuple(_ptable);
    W_DO(iter->next(db, eof, tuple));

    int ituple_cnt=0;
    int idx_cnt=0;

    while (!eof) {
        // remember the rid just scanned
        rid_t tablerid = tuple.rid();

        ituple_cnt++;

        // probe all indexes
        pindex = _ptable->indexes();
        while (pindex) {

            idx_cnt++;

            w_rc_t rc = index_probe(db, pindex, &tuple);

            if (rc.is_error()) {
                TRACE( TRACE_ALWAYS, "Index probe error in (%s) (%s) (%d)\n", 
                       _ptable->name(), pindex->name(), tablerid);
                cerr << "Due to " << rc << endl;
                return RC(se_INCONSISTENT_INDEX);
            }            

            if (tablerid != tuple.rid()) {
                TRACE( TRACE_ALWAYS, "Inconsistent index... (%d)-(%d)",
                       tablerid, tuple.rid());
                return RC(se_INCONSISTENT_INDEX);
            }
            pindex = pindex->next();
        }

	W_DO(iter->next(db, eof, tuple));
    }
    delete (iter);

    W_DO(db->commit_xct());
    time_t tstop = time(NULL);

    TRACE( TRACE_DEBUG, "Indexes on table (%s) found consistent in (%d) secs...\n",
           _ptable->name(), tstop-tstart);
    
    return (RCOK);

}


/******************************************************************** 
 *
 *  @fn:    check_all_indexes
 *
 *  @brief: Check all indexes
 *
 ********************************************************************/

template <class TableDesc>
bool table_man_impl<TableDesc>::check_all_indexes(ss_m* db)
{
    assert (_ptable);

    index_desc_t* pindex = _ptable->indexes();

    TRACE( TRACE_DEBUG, "Checking consistency of the indexes on table (%s)\n",
           _ptable->name());

    while (pindex) {
	w_rc_t rc = check_index(db, pindex);
	if (rc.is_error()) {
            TRACE( TRACE_ALWAYS, "Index checking error in (%s) (%s)\n", 
                   _ptable->name(), pindex->name());
	    cerr << "Due to " << rc << endl;
	    return (false);
	}
	pindex = pindex->next();
    }
    return (true);
}


/********************************************************************* 
 *
 *  @fn:    check_index
 *
 *  @brief: Checks all the values on an index. It first gets the rid from
 *          the table (by scanning) and then probes the index for the same
 *          tuple. It reports error if the two rids do not match.
 *
 *********************************************************************/

template <class TableDesc>
w_rc_t table_man_impl<TableDesc>::check_index(ss_m* db,
                                              index_desc_t* pindex)
{
    assert (_ptable);

    TRACE( TRACE_DEBUG, "Start to check index (%s)\n", pindex->name());

    W_DO(db->begin_xct());

    table_iter* iter;
    W_DO(get_iter_for_file_scan(db, iter));

    bool eof = false;
    table_tuple tuple(_ptable);
    W_DO(iter->next(db, eof, tuple));
    while (!eof) {
        // remember the rid just scanned
        rid_t tablerid = tuple.rid();
	W_DO(index_probe(db, pindex, &tuple));
	if (tablerid != tuple.rid()) {
            TRACE( TRACE_ALWAYS, "Inconsistent index... (%d)-(%d)",
                   tablerid, tuple.rid());
            return RC(se_INCONSISTENT_INDEX);
	}
	W_DO(iter->next(db, eof, tuple));
    }
    delete (iter);

    W_DO(db->commit_xct());
    return (RCOK);
}



/* ------------------ */
/* --- scan index --- */
/* ------------------ */


/********************************************************************* 
 *
 *  @fn:    scan_all_indexes
 *
 *  @brief: Scan all indexes
 *
 *********************************************************************/

template <class TableDesc>
w_rc_t table_man_impl<TableDesc>::scan_all_indexes(ss_m* db)
{
    assert (_ptable);

    index_desc_t* pindex = _ptable->indexes();
    while (pindex) {
	W_DO(scan_index(db, pindex));
	pindex = pindex->next();
    }
    return (RCOK);
}


/********************************************************************* 
 *
 *  @fn:    scan_index
 *
 *  @brief: Iterates over all the values on an index
 *
 *********************************************************************/

template <class TableDesc>
w_rc_t table_man_impl<TableDesc>::scan_index(ss_m* db, 
                                             index_desc_t* pindex)
{
    assert (_ptable);
    assert (pindex);
    assert (!pindex->is_partitioned());

    TRACE( TRACE_DEBUG, "Scanning index (%s) for table (%s)\n", 
           pindex->name(), _ptable->name());

    /* 1. open a index scanner */
    index_iter* iter;

    table_tuple lowtuple(_ptable);
    rep_row_t lowrep(_pts);
    int lowsz = min_key(pindex, &lowtuple, lowrep);
    assert (lowrep._dest);

    table_tuple hightuple(_ptable);
    rep_row_t highrep(_pts);
    int highsz = max_key(pindex, &hightuple, highrep);
    assert (highrep._dest);

    W_DO(get_iter_for_index_scan(db, pindex, iter,
                                 SH, 
                                 false,
				 scan_index_i::ge,
				 vec_t(lowrep._dest, lowsz),
				 scan_index_i::le,
				 vec_t(highrep._dest, highsz)));

    /* 2. iterate over all index records */
    bool        eof;
    int         count = 0;    
    table_tuple row(_ptable);

    W_DO(iter->next(db, eof, row));
    while (!eof) {	
	pin_i  pin;
	W_DO(pin.pin(row.rid(), 0));
	if (!load(&row, pin.body())) return RC(se_WRONG_DISK_DATA);
	pin.unpin();
        row.print_values();

	W_DO(iter->next(db, eof, row));
	count++;
    }
    delete iter;

    /* 3. print out some statistics */
    TRACE( TRACE_DEBUG, "%d tuples found!\n", count);
    TRACE( TRACE_DEBUG, "Scan finished!\n");

    return (RCOK);
}


/* ----------------- */
/* --- debugging --- */
/* ----------------- */

template <class TableDesc>
w_rc_t table_man_impl<TableDesc>::print_table(ss_m* db)
{
    assert (_ptable);

    char   filename[MAX_FILENAME_LEN];
    strcpy(filename, _ptable->name());
    strcat(filename, ".tbl.tmp");
    ofstream fout(filename);

    W_DO(db->begin_xct());

    table_iter* iter;
    int count = 0;
    W_DO(get_iter_for_file_scan(db, iter));

    bool eof = false;
    table_tuple row(_ptable);
    W_DO(iter->next(db, eof, row));
    while (!eof) {
        //	row.print_value(fout);
        //        row.print_tuple();
        count++;
	W_DO(iter->next(db, eof, row));
    }
    delete iter;

    W_DO(db->commit_xct());

    fout << "Table : " << _ptable->name() << endl;
    fout << "Tuples: " << count << endl;

    TRACE( TRACE_DEBUG, "Table (%s) printed (%d) tuples\n",
           _ptable->name(), count);

    return (RCOK);
}





EXIT_NAMESPACE(shore);

#endif /* __SHORE_TABLE_MANAGER_H */
