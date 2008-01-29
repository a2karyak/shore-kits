/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_index.h
 *
 *  @brief:  Description of an index.
 *
 *  All the secondary indexes on the table are linked together.  
 *  An index is described by an array of serial number of fields.
 *
 *  @author: Mengzhi Wang, April 2001
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#ifndef __SHORE_INDEX_H
#define __SHORE_INDEX_H

#include "sm_vas.h"
#include "util.h"

#include "sm/shore/shore_error.h"
#include "sm/shore/shore_file_desc.h"
#include "sm/shore/shore_iter.h"


ENTER_NAMESPACE(shore);


class index_scan_iter_impl;


/* ---------------------------------------------------------------
 * @class: index_desc_t
 *
 * @brief: Description of a Shore index.
 *---------------------------------------------------------------- */

class index_desc_t : public file_desc_t {
    friend class index_scan_iter_impl;

private:
    int*          _key;               /* index of fields in the index */
    bool          _unique;            /* whether allow duplicates or not */
    bool          _primary;           /* it is primary or not */

    index_desc_t* _next;              /* linked list of all indices */
    //    table_desc_t* _table;             /* pointer to the base table */

public:

    /* ------------------- */
    /* --- constructor --- */
    /* ------------------- */

    index_desc_t(const char* name, int fieldcnt, const int* fields,
                 bool unique = true, bool primary = false)
        : file_desc_t(name, fieldcnt),
          _unique(unique), _primary(primary), _next(NULL) 
    {
        // Copy the indexes of keys
        _key = new int[_field_count];
        for (int i=0; i<_field_count; i++) _key[i] = fields[i];
    }

    ~index_desc_t() { 
        if (_key)
            delete [] _key; 
        
        if (_next)
            delete _next;
    }



    /* ---------------------- */
    /* --- access methods --- */
    /* ---------------------- */

    bool        is_unique() const { return _unique; }
    bool        is_primary() const { return _primary; }



    /* ---------------------------- */
    /* --- link list operations --- */
    /* ---------------------------- */


    index_desc_t* next() const { return _next; }

    /* total number of indexes on the table */
    int index_count() const { 
        return (_next ? _next->index_count()+1 : 1); 
    }

    /* insert a new index after the current index */
    void insert(index_desc_t* new_node) {
	new_node->_next = _next;
	_next = new_node;
    }

    /* find the index_desc_t by name */
    index_desc_t* find_by_name(const char* name) {
	if (strcmp(name, _name) == 0) return (this);
	if (_next) return _next->find_by_name(name);
	return NULL;
    }


    /* --- index manipulation --- */

    int key_index(int index) const {
	assert (index >=0 && index < _field_count);
	return _key[index];
    }

}; // EOF: index_desc_t



/* ---------------------------------------------------------------------
 * @class: index_scan_iter_impl
 * @brief: Declaration of a index scan iterator
 * --------------------------------------------------------------------- */

typedef tuple_iter_t<index_desc_t, scan_index_i> index_scan_iter_t;

class index_scan_iter_impl : public index_scan_iter_t /* index_scan_iter_t<index_desc_t> */
{
private:
    bool _need_tuple;

public:

    /* -------------------- */
    /* --- construction --- */
    /* -------------------- */
        
    index_scan_iter_impl(ss_m* db,
                         index_desc_t* index,
                         bool need_tuple = false)
        : index_scan_iter_t(db, index), _need_tuple(need_tuple) 
    { 
        /** @note We need to know the bounds of the iscan before
         *        opening the iterator. That's why we cannot open
         *        the iterator upon construction.
         *        Needs explicit call to open_sca(...)
         */            
    }

    
    /*
    index_scan_iter_impl(ss_m* db,
                         index_desc_t* index,
                         scan_index_i::cmp_t c1, const cvec_t& bound1,
                         scan_index_i::cmp_t c2, const cvec_t& bound2,
                         const int maxkeysize
                         bool need_tuple = false)
        : index_scan_iter_t(db, index), _need_tuple(need_tuple) 
    { W_COERCE(open_scan(db, c1, bound1, c2, bound2, maxkeysize)); }
    */
        
    ~index_scan_iter_impl() { 
        close_scan();             
    }


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

    w_rc_t next(ss_m* db, bool& eof) 
    {
        assert(_opened);

        W_DO(_scan->next(eof));

        if (!eof) {

            // @@@@@@@@@@@@@@@@
            // should fix the code below it does not make sense
            // to use the table...
            // @@@@@@@@@@@@@@@@

            assert (false); 

//             rid_t   rid;
//             vec_t   key(_tuple->format_key(_file), _tuple->key_size(_index));
//             vec_t   record(&rid, sizeof(rid_t));
//             smsize_t klen = 0;
//             smsize_t elen = sizeof(rid_t);
//             W_DO(_scan->curr(&key, klen, &record, elen));
//             _tuple->set_rid(rid);
//             _tuple->load_keyvalue(key.ptr(0), _index);	
//             if (_need_tuple) {
//                 pin_i  pin;
//                 W_DO(pin.pin(rid, 0));
//                 if (!_tuple->load(pin.body()))
//                     return RC(se_WRONG_DISK_DATA);
//                 pin.unpin();
//             }

        }
    
        return RCOK;
    }

}; // EOF: index_scan_iter_impl



EXIT_NAMESPACE(shore);

#endif /* __SHORE_INDEX_H */
