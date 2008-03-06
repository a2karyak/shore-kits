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


class table_desc_t;


/******************************************************************
 *
 *  @class: index_desc_t
 *
 *  @brief: Description of a Shore index.
 *
 *  @note:  Even the variable length fields are treated as fixed 
 *          length, with their maximum possible size.
 *
 ******************************************************************/

class index_desc_t : public file_desc_t 
{
    friend class table_desc_t;

private:

    int*          _key;                      /* index of fields in the base table */
    bool          _unique;                   /* whether allow duplicates or not */
    bool          _primary;                  /* it is primary or not */

    index_desc_t* _next;                     /* linked list of all indices */

    char          _keydesc[MAX_KEYDESC_LEN]; /* buffer for the index key description */
    tatas_lock    _keydesc_lock;             /* lock for the key desc */
    int           _maxkeysize;               /* maximum key size */
    tatas_lock    _maxkey_lock;              /* lock for the maximum key size */

public:

    /* ------------------- */
    /* --- constructor --- */
    /* ------------------- */

    index_desc_t(const char* name, const int fieldcnt, const int* fields,
                 bool unique = true, bool primary = false)
        : file_desc_t(name, fieldcnt),
          _unique(unique), _primary(primary), _next(NULL), 
          _maxkeysize(0) 
    {
        // Copy the indexes of keys
        _key = new int[_field_count];
        for (int i=0; i<_field_count; i++) _key[i] = fields[i];

        memset(_keydesc, 0, MAX_KEYDESC_LEN);
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

    inline bool is_unique() const { return (_unique); }
    inline bool is_primary() const { return (_primary); }

    inline int  get_keysize() { return (_maxkeysize); }
    inline void set_keysize(const int sz) {
        assert (sz>0);
        CRITICAL_SECTION(idx_keysz_cs, _maxkey_lock);
        _maxkeysize = sz;
    }

    /* ---------------------------------- */
    /* --- index link list operations --- */
    /* ---------------------------------- */


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
	return (NULL);
    }


    /* --- index manipulation --- */

    int key_index(int index) const {
	assert (index >=0 && index < _field_count);
	return (_key[index]);
    }

}; // EOF: index_desc_t



EXIT_NAMESPACE(shore);

#endif /* __SHORE_INDEX_H */
