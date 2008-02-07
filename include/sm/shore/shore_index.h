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



/* ---------------------------------------------------------------
 * @class: index_desc_t
 *
 * @brief: Description of a Shore index.
 *---------------------------------------------------------------- */

class index_desc_t : public file_desc_t 
{
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



EXIT_NAMESPACE(shore);

#endif /* __SHORE_INDEX_H */
