/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
               Ecole Polytechnique Federale de Lausanne

                       Copyright (c) 2007-2008
                      Carnegie-Mellon University

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

class index_desc_t 
{
    friend class table_desc_t;

private:
    file_desc_t _base;

    int*            _key;                      /* index of fields in the base table */
    bool            _unique;                   /* whether allow duplicates or not */
    bool            _primary;                  /* is it primary or not */

    index_desc_t*   _next;                     /* linked list of all indices */

    char            _keydesc[MAX_KEYDESC_LEN]; /* buffer for the index key description */
    tatas_lock      _keydesc_lock;             /* lock for the key desc */
    volatile uint_t _maxkeysize;               /* maximum key size */

    /* if _partition_count > 1, _partition_stids contains stids for
       all N partitions, otherwise it's NULL
     */
    int		  _partition_count;
    stid_t*	  _partition_stids;

public:

    /* ------------------- */
    /* --- constructor --- */
    /* ------------------- */

    index_desc_t(const char* name, const int fieldcnt, int partitions, const int* fields,
                 bool unique=true, bool primary=false)
        : _base(name, fieldcnt),
          _unique(unique), _primary(primary),
          _next(NULL), _maxkeysize(0),
	  _partition_count((partitions > 0)? partitions : 1), _partition_stids(0)
    {
        // Copy the indexes of keys
        _key = new int[_base._field_count];
        for (int i=0; i<_base._field_count; i++) _key[i] = fields[i];

        memset(_keydesc, 0, MAX_KEYDESC_LEN);

	// start setting up partitioning
	if(is_partitioned()) {
	    _partition_stids = new stid_t[_partition_count];
	    for(int i=0; i < _partition_count; i++)
		_partition_stids[i] = stid_t::null;
	}
    }

    ~index_desc_t() { 
        if (_key)
            delete [] _key; 
        
        if (_next)
            delete _next;
	
	if(_partition_stids)
	    delete [] _partition_stids;
    }


    /* --------------------------------- */
    /* --- exposed inherited methods --- */
    /* --------------------------------- */

    const char*	name() const { return _base.name(); }
    const int   field_count() const { return _base.field_count(); }
    


    /* -------------------------- */
    /* --- overridden methods --- */
    /* -------------------------- */

    inline w_rc_t check_fid(ss_m* db) {
	if(!is_partitioned())
	    return _base.check_fid(db);
	
	// check all the partition fids...
	for(int i=0; i < _partition_count; i++) {
	    if (!is_fid_valid(i)) {
		if (!_base.is_root_valid())
		    W_DO(_base.find_root_iid(db));
		W_DO(find_fid(db, i));
	    }
        }
        return (RCOK);
    }

    stid_t&	fid(int pnum) {
	assert(pnum >= 0 && pnum < _partition_count);
	return is_partitioned()? _partition_stids[pnum] : _base.fid();
    }	
    w_rc_t	find_fid(ss_m* db, int pnum);
    
    bool is_fid_valid(int pnum) const {
	assert(pnum >= 0 && pnum < _partition_count);
	return is_partitioned()? _partition_stids[pnum] != stid_t::null : _base.is_fid_valid();
    }
    void set_fid(int pnum, stid_t const &fid) {
	assert(pnum >= 0 && pnum < _partition_count);
	if(is_partitioned())
	    _partition_stids[pnum] = fid;
	else
	    _base.set_fid(fid);
    }
	
    
    /* ---------------------- */
    /* --- access methods --- */
    /* ---------------------- */

    inline bool is_unique() const { return (_unique); }
    inline bool is_primary() const { return (_primary); }
    inline bool is_partitioned() const { return _partition_count > 1; }

    inline int  get_partition_count() const { return _partition_count; }
    inline int  get_keysize() { return (*&_maxkeysize); }
    inline void set_keysize(const uint_t sz) { atomic_swap_uint(&_maxkeysize, sz); }

    /* ---------------------------------- */
    /* --- index link list operations --- */
    /* ---------------------------------- */


    index_desc_t* next() const { return _next; }

    // total number of indexes on the table
    int index_count() const { 
        return (_next ? _next->index_count()+1 : 1); 
    }

    // insert a new index after the current index
    void insert(index_desc_t* new_node) {
	new_node->_next = _next;
	_next = new_node;
    }

    // find the index_desc_t by name
    index_desc_t* find_by_name(const char* name) {
	if (strcmp(name, _base._name) == 0) return (this);
	if (_next) return _next->find_by_name(name);
	return (NULL);
    }


    /* --- index manipulation --- */

    int key_index(int index) const {
	assert (index >=0 && index < _base._field_count);
	return (_key[index]);
    }

}; // EOF: index_desc_t



EXIT_NAMESPACE(shore);

#endif /* __SHORE_INDEX_H */
