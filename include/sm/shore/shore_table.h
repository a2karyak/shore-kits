/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_table.h
 *
 *  @brief:  Base class for tables stored in Shore
 *
 *  @note:   typle_desc_t - table abstraction
 *           typle_row_t  - row of a table
 *
 *  @author: Mengzhi Wang, April 2001
 *  @author: Ippokratis Pandis, January 2008
 *
 */


/* Tuple.h contains the base class (table_desc_t) for tables stored in
 * Shore. The table consists of several parts:
 *
 * 1. An array of field_desc, which contains the decription of the
 * fields.  The number of fields is set by the constructor. The schema
 * of the table is not written to the disk.  
 * 
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * @note  Modifications to the schema need rebuilding the whole
 *        database.
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 * 2. The primary index of the table. 
 *
 * 3. Secondary indices on the table.  All the secondary indices created
 * on the table are stored as a linked list.
 *
 * 4. Physical id of the current tuple.  (Record id type: rid_t,
 * defined in Shore.)
 *
 * There are methods in tuple_desc for creation, loading of the table
 * and indexes.  Operation on a single tuple, including adding,
 * updating, and index probe is provided as well.
 *
 * The file scan and index scan on the table are implemented through
 * two subclasses, tuple_iter_file_scan and tuple_iter_index_scan,
 * which are inherited from tuple_iter.
 *
 * USAGE:
 *
 * To create a new table, create a class for the table by inheriting
 * publicly from class tuple_desc to take advantage of all the
 * built-in tools. The schema of the table should be set at the
 * constructor of the table.  (See tpcc_table.h for examples.)
 *
 * NOTE:
 *
 * Due to the limitation of Shore implementation, only the last field
 * in indices can be variable length.
 *
 * BUGS:
 *
 * If a new index is created on an existing table, explicit call to
 * load the index is needed.
 *
 * Timestamp field is not fully implemented: no set function.
 *
 * EXTENSIONS:
 *
 * The mapping between SQL types and C types are defined in
 * field_desc_t.  Modify the class to support more SQL types or
 * change the mapping.  The NUMERIC type is currently stored as string;
 * no further understanding is provided yet.  */

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
 * beginning of the tuple (address of a).  */

#ifndef __SHORE_TABLE_H
#define __SHORE_TABLE_H


#include "util.h"
#include "sm_vas.h"

#include "shore_msg.h"
#include "shore_error.h"
#include "shore_file_desc.h"
#include "shore_tools.h"
#include "shore_field.h"
#include "shore_iter.h"
#include "shore_index.h"


ENTER_NAMESPACE(shore);


struct table_row_t;
class table_scan_iter_impl;
class index_scan_iter_impl;


/* ---------------------------------------------------------------
 * @class: table_desc_t
 *
 * @brief: Description of a Shore table.
 *
 * --------------------------------------------------------------- */

class table_desc_t : public file_desc_t {
    friend class table_scan_iter_impl;
   
protected:

    /* ------------------- */
    /* --- table schema -- */
    /* ------------------- */

    field_desc_t*  _desc;                     /* schema - set of field descriptors */
    char           _keydesc[MAX_KEYDESC_LEN]; /* buffer for key descriptions */

    index_desc_t*  _primary_idx;              /* pointer to primary idx */
    index_desc_t*  _indexes;                  /* indices on the table */


    int            find_field(const char* field_name) const;
    index_desc_t*  find_index(const char* name) { 
        return (_indexes ? _indexes->find_by_name(name) : NULL); 
    }

public:

    /* ------------------- */
    /* --- Constructor --- */
    /* ------------------- */

    table_desc_t(const char* name, int fieldcnt)
        : file_desc_t(name, fieldcnt), _primary_idx(NULL), _indexes(NULL) 
    {
        // Create placeholders for the field descriptors
	_desc = new field_desc_t[fieldcnt];
    }
    
    virtual ~table_desc_t() 
    {
        if (_desc)
            delete [] _desc;

        if (_indexes)
            delete _indexes;
    }



    /* --------------------------------------- */
    /* --- create physical table and index --- */
    /* --------------------------------------- */

    w_rc_t  create_table(ss_m* db);


    /* create an index on the table (this only creates the index
     * decription for the index in memory. call bulkload_index to
     * create and populate the index on disks.
     */
    bool    create_index(const char* name,
			 const int* fields,
			 const int num,
			 const bool unique=true,
                         const bool primary=false);

    bool    create_primary_idx(const char* name,
                               const int* fields,
                               const int num);



    /* ------------------------ */
    /* --- index facilities --- */
    /* ------------------------ */

    index_desc_t* find_index_by_name(const char* name) { 
        return (_indexes ? _indexes->find_by_name(name) : NULL); 
    }
    
    int index_count() { return _indexes->index_count(); } /* # of indexes */

    index_desc_t* primary() { return (_primary_idx); }

    /* sets primary index, the index should be already set to
     * primary and unique */
    void set_primary(index_desc_t* idx) { 
        assert (idx->is_primary() && idx->is_unique());
        _primary_idx = idx; 
    }


    /* -------------------------------------------------- */
    /* --- bulk-building the specified index on disks --- */
    /* -------------------------------------------------- */

    w_rc_t  bulkload_all_indexes(ss_m* db);
    w_rc_t  bulkload_index(ss_m* db, const char* name);
    w_rc_t  bulkload_index(ss_m* db, index_desc_t* idx);

    /* check consistency between the indexes and table
     *   true:  consistent
     *   false: inconsistent
     */
    bool   check_all_indexes(ss_m* db);
    w_rc_t check_index(ss_m* db, index_desc_t* idx);
    w_rc_t scan_all_indexes(ss_m* db);
    w_rc_t scan_index(ss_m* db, index_desc_t* idx);


    /* ------------------------------------------------ */
    /* --- helper functions for bulkloading indices --- */
    /* ------------------------------------------------ */

    char* index_keydesc(index_desc_t* index);                            /* index key description */
    char* format_key(index_desc_t* index, table_row_t* prow);            /* format the key value */
    char* min_key(index_desc_t* index, table_row_t* prow);               /* set indexed fields of the row to minimum */
    char* max_key(index_desc_t* index, table_row_t* prow);               /* set indexed fields of the row to maximum */
    int   key_size(index_desc_t* index, const table_row_t* prow) const;  /* length of the formatted key */
    int   maxkeysize(index_desc_t* index) const;                         /* max key size */


    /* ---------------------- */
    /* --- access methods --- */
    /* ---------------------- */


    /* find the tuple through index */

    /* based on idx id */
    w_rc_t   index_probe(ss_m* db,
                         index_desc_t* idx,
                         table_row_t* tuple,
                         lock_mode_t lmode = SH); /* one of: SH, EX */

    w_rc_t   index_probe_forupdate(ss_m* db,
                                   index_desc_t* idx,
                                   table_row_t* row)
    {
        return (index_probe(db, idx, row, EX));
    }

    /* probe primary idx */
    w_rc_t   probe_primary(ss_m* db, table_row_t* row) 
    {
        assert (_primary_idx);
        return (index_probe(db, _primary_idx, row));
    }

    /* based on idx name */
    w_rc_t   index_probe(ss_m* db, const char* idx_name, table_row_t* row) 
    {
        index_desc_t* index = find_index(idx_name);
        assert(index);
        return (index_probe(db, index, row));
    }

    w_rc_t   index_probe_forupdate(ss_m* db, 
                                   const char* idx_name,
                                   table_row_t* row) 
    { 
	index_desc_t* pindex = find_index(idx_name);
	assert(pindex);
	return index_probe_forupdate(db, pindex, row);
    }



    /* -------------------------- */
    /* --- tuple manipulation --- */
    /* -------------------------- */

    w_rc_t    add_tuple(ss_m* db, table_row_t* tuple);
    w_rc_t    update_tuple(ss_m* db, table_row_t* tuple);
    w_rc_t    delete_tuple(ss_m* db, table_row_t* tuple);



    /* ------------------------------------------- */
    /* --- iterators for index and table scans --- */
    /* ------------------------------------------- */

    w_rc_t get_iter_for_file_scan(ss_m* db,
				  table_scan_iter_impl* &iter);

    w_rc_t get_iter_for_index_scan(ss_m* db,
				   index_desc_t* index,
				   index_scan_iter_impl* &iter,
				   scan_index_i::cmp_t c1,
				   const cvec_t & bound1,
				   scan_index_i::cmp_t c2,
				   const cvec_t & bound2,
				   bool need_tuple = false);
    


    /* ----------------------------------------------- */
    /* --- populate the table with data from files --- */
    /* ----------------------------------------------- */

    w_rc_t       load_from_file(ss_m* db, const char* fname = NULL);
    virtual bool read_tuple_from_line(table_row_t& tuple, char* string)=0;


    /* -------------------------------------------------------- */
    /* --- conversion between disk format and memory format --- */
    /* -------------------------------------------------------- */

    int maxsize() const;                 /* maximum requirement for disk format */

    inline field_desc_t* desc(int descidx) {
        assert (descidx >= 0 && descidx < _field_count);
        assert (_desc);
        return (&(_desc[descidx]));
    }


    /* ----------------------- */
    /* --- debugging tools --- */
    /* ----------------------- */

    w_rc_t print_table(ss_m* db);                 /* print the table on screen */
    void   print_desc(ostream & os = cout);     /* print the schema */


}; // EOF: table_desc_t


typedef std::list<table_desc_t*> table_list_t;



/* ---------------------------------------------------------------
 * @struct: table_row_t
 *
 * @brief: Represents a row (record) of a table. It is used instead
 *         so that multiple threads can operate concurrently on a
 *         table
 *
 * --------------------------------------------------------------- */

struct table_row_t {
    
    table_desc_t*  _ptable;           /* pointer back to the table */
    int            _field_cnt;        /* number of fields */
    bool           _is_setup;         /* flag if already setup */
    
    rid_t          _rid;              /* record id */    
    field_value_t* _pvalues;          /* set of values */

    char*          _formatted_data;   /* buffer for tuple in disk format */


    /* -------------------- */
    /* --- construction --- */
    /* -------------------- */

    table_row_t()
        : _ptable(NULL), _field_cnt(0), _is_setup(false), 
          _rid(rid_t::null), _pvalues(NULL), _formatted_data(NULL)
    {
    }
        

    table_row_t(table_desc_t* ptd)
        : _ptable(NULL), _field_cnt(0), _is_setup(false), 
          _rid(rid_t::null), _pvalues(NULL), _formatted_data(NULL)
    {
        setup(ptd);
    }

    
    ~table_row_t() 
    {
        if (_pvalues)
            delete [] _pvalues;

        if (_formatted_data)
            delete [] _formatted_data;
    }


    /* setup row according to table description, asserts if NULL */
    void setup(table_desc_t* ptd) 
    {
        assert (ptd);

        // if it is already setup for this table do nothing
        if (_is_setup && _ptable == ptd)
            return;

        _ptable = ptd;
        _field_cnt = ptd->field_count();
        assert (_field_cnt>0);

        _pvalues = new field_value_t[_field_cnt];
        for (int i=0; i<_field_cnt; i++)
            _pvalues[i].setup(ptd->desc(i));

        _is_setup = true;
    }



    /* -------------------------------------------------------- */
    /* --- conversion between disk format and memory format --- */
    /* -------------------------------------------------------- */

    char* buffer() {
        assert (_ptable);

	if (!_formatted_data) 
            _formatted_data = new char[_ptable->maxsize()];
	return (_formatted_data);
    }

    const char* format();            /* disk format of tuple */

    int   size() const;             /* disk space needed for tuple */
    bool  load(const char* string); /* load tuple from disk format */ 

    bool  load_keyvalue(const unsigned char* string,
                        index_desc_t* index); /* load key fields */

    char* format_key(index_desc_t* index); 
    int   key_size(index_desc_t* index) const;


    /* ---------------------- */
    /* --- access methods --- */
    /* ---------------------- */

    inline rid_t   rid() const { return (_rid); }
    inline void    set_rid(const rid_t& rid) { _rid = rid; }
    inline bool    is_rid_valid() const { return _rid != rid_t::null; }


    /* ------------------------ */
    /* --- set field values --- */
    /* ------------------------ */
    
    inline void set_null(int idx) 
    {
	assert (idx >= 0 && idx < _field_cnt);
        assert (_is_setup && _pvalues[idx].is_setup());
	_pvalues[idx].set_null();
    }

    inline void set_value(int idx, const int v) 
    {
	assert (idx >= 0 && idx < _field_cnt);
        assert (_is_setup && _pvalues[idx].is_setup());
	_pvalues[idx].set_int_value(v);
    }

    inline void set_value(int idx, const short v) 
    {
	assert (idx >= 0 && idx < _field_cnt);
        assert (_is_setup && _pvalues[idx].is_setup());
	_pvalues[idx].set_smallint_value(v);
    }

    inline void set_value(int idx, const double v) 
    {
	assert (idx >= 0 && idx < _field_cnt);
        assert (_is_setup && _pvalues[idx].is_setup());
	_pvalues[idx].set_value(&v, 0);
    }

    inline void set_value(int idx, const decimal v) 
    {
	assert (idx >= 0 && idx < _field_cnt);
        assert (_is_setup && _pvalues[idx].is_setup());
	_pvalues[idx].set_decimal_value(v);
    }

    inline void set_value(int idx, const time_t v) 
    {
	assert (idx >= 0 && idx < _field_cnt);
        assert (_is_setup && _pvalues[idx].is_setup());
	_pvalues[idx].set_time_value(v);
    }

    inline void set_value(int idx, const char* string) 
    {
	assert (idx >= 0 && idx < _field_cnt);
        assert (_is_setup && _pvalues[idx].is_setup());
        register field_desc_t* pdv = _pvalues[idx].field_desc();

	assert (pdv->type() == SQL_VARCHAR || 
                pdv->type() == SQL_CHAR );

	int len = strlen(string);
	if ( pdv->type() == SQL_CHAR ) { 
            if (len > pdv->maxsize()) {
                len = pdv->maxsize();
            }
	}
	_pvalues[idx].set_value(string, len);
    }

    inline void set_value(int idx, const timestamp_t& time) 
    {
	assert (idx >= 0 && idx < _field_cnt);
        assert (_is_setup && _pvalues[idx].is_setup());
	_pvalues[idx].set_value(&time, 0);
    }


    /* ------------------------- */
    /* --- find field values --- */
    /* ------------------------- */

    bool get_value(const int idx, int& value) const;
    bool get_value(const int idx, short& value) const;
    bool get_value(const int idx, char* buffer, const int bufsize) const;
    bool get_value(const int idx, double& value) const;
    bool get_value(const int idx, decimal& value) const;
    bool get_value(const int idx, time_t& value) const;
    bool get_value(const int idx, timestamp_t& value) const;



    /* --- debugging --- */
    void print_value(ostream& os = cout); /* print the tuple values */
    void print_tuple();                   /* print the whole tuple */
}; // EOF: table_row_t




/* ---------------------------------------------------------------------
 * @class: table_scan_iter_impl
 *
 * @brief: Declaration of a table (file) scan iterator
 *
 * --------------------------------------------------------------------- */

typedef tuple_iter_t<table_desc_t, scan_file_i, table_row_t> table_scan_iter_t;

class table_scan_iter_impl : public table_scan_iter_t 
{
public:

    /* -------------------- */
    /* --- construction --- */
    /* -------------------- */

    table_scan_iter_impl(ss_m* db, 
                         table_desc_t* table) 
        : table_scan_iter_t(db, table)
    { 
        W_COERCE(open_scan(_db)); 
    }
        
    ~table_scan_iter_impl() { close_scan(); }


    /* ------------------------ */
    /* --- fscan operations --- */
    /* ------------------------ */

    w_rc_t open_scan(ss_m* db) {
        if (!_opened) {
            W_DO(_file->check_fid(db));
            _scan = new scan_file_i(_file->fid(), ss_m::t_cc_record);
            _opened = true;
        }
        return (RCOK);
    }

    w_rc_t next(ss_m* db, bool& eof, table_row_t& tuple) {
        if (!_opened) open_scan(db);
        pin_i* handle;
        W_DO(_scan->next(handle, 0, eof));
        if (!eof) {
            if (!tuple.load(handle->body()))
                return RC(se_WRONG_DISK_DATA);
            tuple.set_rid(handle->rid());
        }
        return (RCOK);
    }

}; // EOF: table_scan_iter_impl



/* ---------------------------------------------------------------------
 * @class: index_scan_iter_impl
 * @brief: Declaration of a index scan iterator
 * --------------------------------------------------------------------- */


typedef tuple_iter_t<index_desc_t, scan_index_i, table_row_t> index_scan_iter_t;

class index_scan_iter_impl : public index_scan_iter_t
{
private:
    bool _need_tuple;

public:

    /* -------------------- */
    /* --- construction --- */
    /* -------------------- */
        
    index_scan_iter_impl(ss_m* db,
                         index_desc_t* pindex,
                         bool need_tuple = false)
        : index_scan_iter_t(db, pindex), _need_tuple(need_tuple) 
    { 
        /** @note We need to know the bounds of the iscan before
         *        opening the iterator. That's why we cannot open
         *        the iterator upon construction.
         *        Needs explicit call to open_scan(...)
         */        
    };

    index_scan_iter_impl(ss_m* db,
                         index_desc_t* pindex,
                         scan_index_i::cmp_t cmp1, const cvec_t& bound1,
                         scan_index_i::cmp_t cmp2, const cvec_t& bound2,
                         const int maxkeysize,
                         bool need_tuple) 
        : index_scan_iter_t(db, pindex), _need_tuple(need_tuple)
    { 
        /** @note In case we know the bounds of the iscan a priory */
        W_COERCE(open_scan(db, cmp1, bound1, cmp2, bound2, maxkeysize));
    };

        
    ~index_scan_iter_impl() { 
        close_scan();             
    };


    /* ------------------------ */        
    /* --- iscan operations --- */
    /* ------------------------ */

    w_rc_t open_scan(ss_m* db,
                     scan_index_i::cmp_t c1, const cvec_t& bound1,
                     scan_index_i::cmp_t c2, const cvec_t& bound2,
                     const int maxkeysize)
    {
        if (!_opened) {
            W_DO(_file->check_fid(db));
            _scan = new scan_index_i(_file->fid(), c1, bound1, c2, bound2);
            _opened = true;
        }
        return RCOK;
    }

    w_rc_t next(ss_m* db, bool& eof, table_row_t& tuple) 
    {
        assert(_opened);

        W_DO(_scan->next(eof));

        if (!eof) {
            rid_t    rid;
            vec_t    key(tuple.format_key(_file), tuple.key_size(_file));
            vec_t    record(&rid, sizeof(rid_t));
            smsize_t klen = 0;
            smsize_t elen = sizeof(rid_t);

            W_DO(_scan->curr(&key, klen, &record, elen));
            tuple.set_rid(rid);
            tuple.load_keyvalue(key.ptr(0), _file);

            if (_need_tuple) {
                pin_i  pin;
                W_DO(pin.pin(rid, 0));
                if (!tuple.load(pin.body()))
                    return RC(se_WRONG_DISK_DATA);
                pin.unpin();
            }

        }    
        return (RCOK);
    }

}; // EOF: index_scan_iter_impl



//////////////////////////////////////////////////////////////////////
// 
// class table_desc_t methods 
//
//////////////////////////////////////////////////////////////////////


inline int table_desc_t::find_field(const char* field_name) const
{
    for (int i=0; i<_field_count; i++) {
        if (strcmp(field_name, _desc[i].name())==0) return i;
    }
    return -1;
}


/* Iterates over all the fields of a selected index and returns 
 * on a single string the corresponding key description
 */
inline char* table_desc_t::index_keydesc(index_desc_t* idx)
{
    strcpy(_keydesc, "");
    for (int i=0; i<idx->field_count(); i++) {
        strcat(_keydesc, _desc[idx->key_index(i)].keydesc());
    }
    return _keydesc;
}


/****************************************************************** 
 *
 * @fn:    format_key
 *
 * @brief: Gets an index and for a selected row it copies only the 
 *         fields that are contained in the index and returns the
 *         corresponding char buffer. 
 *
 ******************************************************************/

inline char* table_desc_t::format_key(index_desc_t* index,
                                      table_row_t* prow)
{
    assert (index);
    assert (prow);

    if (!prow->_formatted_data) 
        prow->_formatted_data = new char [maxsize()];
    
    offset_t offset = 0;
    for (int i=0; i<index->field_count(); i++) {
        int ix = index->key_index(i);

        register field_value_t* pfv = &prow->_pvalues[ix];
        register field_desc_t* pfd = pfv->_pfield_desc;
        // copy value
        if (!pfv->copy_value(prow->_formatted_data+offset))
            return NULL;

        // move offset
        if (pfd->is_variable_length()) {
            offset += pfv->realsize();
        }
        else {
            offset += pfd->maxsize();
        }
    }

    return (prow->_formatted_data);
}



/****************************************************************** 
 *
 * @fn:    min_key/max_key
 *
 * @brief: Gets an index and for a selected row it sets all the 
 *         fields that are contained in the index to their 
 *         minimum or maximum value
 *
 ******************************************************************/

inline char* table_desc_t::min_key(index_desc_t* index, table_row_t* prow)
{
    for (int i=0; i<index->field_count(); i++) {
	int field_index = index->key_index(i);
	prow->_pvalues[field_index].set_min_value();
    }
    return format_key(index, prow);
}


inline char* table_desc_t::max_key(index_desc_t* index, table_row_t* prow)
{
    for (int i=0; i<index->field_count(); i++) {
	int field_index = index->key_index(i);
	prow->_pvalues[field_index].set_max_value();
    }
    return format_key(index,prow);
}



/****************************************************************** 
 *
 * @fn:    key_size
 *
 * @brief: Gets an index and for a selected row it returns the 
 *         real or maximum size of the index key for that row
 *
 ******************************************************************/

inline int table_desc_t::key_size(index_desc_t* index, 
                                  const table_row_t* prow) const
{
    int size = 0;
    for (int i=0; i<index->field_count(); i++) {
        int field_index = index->key_index(i);
        if (prow->_pvalues[field_index].is_variable_length()) 
            size += prow->_pvalues[field_index].realsize();
        else 
            size += prow->_pvalues[field_index]._pfield_desc->maxsize();
    }
    return (size);
}


inline int table_desc_t::maxkeysize(index_desc_t* idx) const
{
    int size = 0;
    for (int i=0; i<idx->field_count(); i++) {
        size += _desc[idx->key_index(i)].maxsize();
    }
    return size;
}



/****************************************************************** 
 *
 *  @fn:    maxsize()
 *
 *  @brief: Return the maximum requirement for a tuple in disk format.
 *          Only used in allocating _formatted_data.  Should be called
 *          only once for each table.
 *
 ******************************************************************/

inline int  table_desc_t::maxsize() const
{
    int size = 0;
    int var_count = 0;
    int null_count = 0;
    for (int i=0; i<_field_count; i++) {
        size += _desc[i].maxsize();
        if (_desc[i].allow_null()) null_count++;
        if (_desc[i].is_variable_length()) var_count++;
    }

    return (size + (var_count*sizeof(offset_t)) + (null_count/8) + 1);
}



/******************************************************************
 * 
 * class table_row_t methods 
 *
 ******************************************************************/



/* return the actual size of the tuple in disk format */
inline int table_row_t::size() const
{
    int size = 0;
    int null_count = 0;

    /* for a fixed length field, it just takes as much as the
     * space for the value itself to store.
     * for a variable length field, we store as much as the data
     * and the offset to tell the length of the data.
     * Of course, there is a bit for each nullable field.
     */
    for (int i=0; i<_field_cnt; i++) {
	if (_pvalues[i]._pfield_desc->allow_null()) {
	    null_count++;
	    if (_pvalues[i].is_null()) continue;
	}
	if (_pvalues[i].is_variable_length()) {
	    size += _pvalues[i].realsize();
	    size += sizeof(offset_t);
	}
	else size += _pvalues[i].realsize();
    }
    if (null_count) size += (null_count >> 3) + 1;
    return size;
}


inline char* table_row_t::format_key(index_desc_t* index)
{
    assert (_ptable);
    return (_ptable->format_key(index, this));
}


inline int table_row_t::key_size(index_desc_t* index) const
{
    assert (_ptable);
    return (_ptable->key_size(index, this));
}



/* ------------------------- */
/* --- find field values --- */
/* ------------------------- */


inline bool table_row_t::get_value(const int index,
                                   int& value) const
{
    assert(index >= 0 && index < _field_cnt);
    if (_pvalues[index].is_null()) {
        value = 0;
        return false;
    }
    value = _pvalues[index].get_int_value();
    return true;
}

inline bool table_row_t::get_value(const int index,
                                   short& value) const
{
    assert(index >= 0 && index < _field_cnt);
    if (!_pvalues[index].is_null()) {
        value = _pvalues[index].get_smallint_value();
        return true;
    }
    return false;
}

inline bool table_row_t::get_value(const int index,
                                    char* buffer,
                                    const int bufsize) const
{
    assert(index >= 0 && index < _field_cnt);
    if (!_pvalues[index].is_null()) {
        int size = MIN(bufsize-1, _pvalues[index]._pfield_desc->maxsize());
        _pvalues[index].get_string_value(buffer, size);
        buffer[size] ='\0';
        return true;
    }
    buffer[0] = '\0';
    return false;
}

inline bool table_row_t::get_value(const int index,
                                   double& value) const
{
    assert(index >= 0 && index < _field_cnt);
    if (!_pvalues[index].is_null()) {
        value = _pvalues[index].get_float_value();
        return true;
    }
    return false;
}


inline bool table_row_t::get_value(const int index,
                                   decimal& value) const
{
    assert(index >= 0 && index < _field_cnt);
    if (!_pvalues[index].is_null()) {
        value = _pvalues[index].get_decimal_value();
        return true;
    }
    return false;
}

inline bool table_row_t::get_value(const int index,
                                   time_t& value) const
{
    assert(index >= 0 && index < _field_cnt);
    if (!_pvalues[index].is_null()) {
        value = _pvalues[index].get_time_value();
        return true;
    }
    return false;
}

inline  bool table_row_t::get_value(const int index,
                                    timestamp_t& value) const
{
    assert(index >= 0 && index < _field_cnt);
    if (!_pvalues[index].is_null()) {
        value = _pvalues[index].get_tstamp_value();
        return true;
    }
    return false;
}



EXIT_NAMESPACE(shore);

#endif /* __SHORE_TABLE_H */
