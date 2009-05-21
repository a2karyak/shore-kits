/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_table.h
 *
 *  @brief:  Base class for tables stored in Shore
 *
 *  @note:   table_desc_t - table abstraction
 *
 *  @author: Ippokratis Pandis, January 2008
 *
 */


/* shore_table.h contains the base class (table_desc_t) for tables stored in
 * Shore. Each table consists of several parts:
 *
 * 1. An array of field_desc, which contains the decription of the
 *    fields.  The number of fields is set by the constructor. The schema
 *    of the table is not written to the disk.  
 *
 * 2. The primary index of the table. 
 *
 * 3. Secondary indices on the table.  All the secondary indices created
 *    on the table are stored as a linked list.
 *
 *
 * FUNCTIONALITY
 *
 * There are methods in (table_desc_t) for creating, the table
 * and indexes. 
 *
 * 
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * @note  Modifications to the schema need rebuilding the whole
 *        database.
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *
 * USAGE:
 *
 * To create a new table, create a class for the table by inheriting
 * publicly from class tuple_desc_t to take advantage of all the
 * built-in tools. The schema of the table should be set at the
 * constructor of the table.  (See shore_tpcc_schema.h for examples.)
 *
 *
 * NOTE:
 *
 * Due to limitation of Shore implementation, only the last field
 * in indexes can be variable length.
 *
 *
 * BUGS:
 *
 * If a new index is created on an existing table, explicit call to
 * load the index is needed.
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

#ifndef __SHORE_TABLE_H
#define __SHORE_TABLE_H


#include "util.h"
#include "sm_vas.h"

#include "shore_msg.h"
#include "shore_error.h"
#include "shore_file_desc.h"
#include "shore_tools.h"
#include "shore_field.h"
#include "shore_index.h"
#include "shore_row.h"


ENTER_NAMESPACE(shore);


/* ---------------------------------------------------------------
 *
 * @class: table_desc_t
 *
 * @brief: Description of a Shore table. Gives access to the fields,
 *         and indexes of the table.
 *
 * --------------------------------------------------------------- */

class table_desc_t : public file_desc_t 
{
protected:

  /* ------------------- */
  /* --- table schema -- */
  /* ------------------- */
  
  field_desc_t*   _desc;               // schema - set of field descriptors
  
  index_desc_t*   _indexes;            // indexes on the table
  index_desc_t*   _primary_idx;        // pointer to primary idx
  
  index_desc_t*   _nolock_primary_idx; // (optional) nolock primary index

  volatile uint_t _maxsize;            // max tuple size for this table, shortcut

  int find_field_by_name(const char* field_name) const;

public:

    /* ------------------- */
    /* --- Constructor --- */
    /* ------------------- */

    table_desc_t(const char* name, int fieldcnt)
        : file_desc_t(name, fieldcnt), 
          _indexes(NULL), 
          _primary_idx(NULL), _nolock_primary_idx(NULL),
          _maxsize(0)
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

    w_rc_t create_table(ss_m* db);


    /* create an index on the table (this only creates the index
     * decription for the index in memory. call bulkload_index to
     * create and populate the index on disks.
     */
    bool   create_index(const char* name,
			int partitions,
                        const int* fields,
                        const int num,
                        const bool unique=true,
                        const bool primary=false,
                        const bool nolock=false);

    bool   create_primary_idx(const char* name,
			      int partitions,
                              const int* fields,
                              const int num,
                              const bool nolock=false);


    /* ------------------------ */
    /* --- index facilities --- */
    /* ------------------------ */

    index_desc_t* indexes() { return (_indexes); }

    // index by name
    index_desc_t* find_index(const char* index_name) { 
        return (_indexes ? _indexes->find_by_name(index_name) : NULL); 
    }
    
    // # of indexes
    int index_count() { return (_indexes->index_count()); } 

    index_desc_t* primary() { return (_primary_idx); }

    /* sets primary index, the index itself should be already set to
     * primary and unique */
    void set_primary(index_desc_t* idx) { 
        assert (idx->is_primary() && idx->is_unique());
        _primary_idx = idx; 
    }

    /* sets primary no-lock index, the index itself should be already set to
     * nolock and unique */
    void set_nolock_primary(index_desc_t* idx) { 
        assert (idx->is_unique() && idx->is_relaxed());
        _nolock_primary_idx = idx; 
    }

    const char* index_keydesc(index_desc_t* idx);
    const int   index_maxkeysize(index_desc_t* index) const; /* max index key size */


    /* ---------------------------------------------------------------- */
    /* --- for the conversion between disk format and memory format --- */
    /* ---------------------------------------------------------------- */

    const int maxsize(); /* maximum requirement for disk format */

    inline field_desc_t* desc(int descidx) {
        assert ((descidx>=0) && (descidx<_field_count));
        assert (_desc);
        return (&(_desc[descidx]));
    }


    /* ------------------------------------------ */
    /* --- populate table with data from file --- */
    /* ------------------------------------------ */

    virtual bool read_tuple_from_line(table_row_t& tuple, char* buf) {
        assert (0); // should not be called. used to be pure abstract
        return (false);
    }
    


    /* ----------------- */
    /* --- debugging --- */
    /* ----------------- */

    void print_desc(ostream & os = cout);  /* print the schema */

}; // EOF: table_desc_t


typedef std::list<table_desc_t*> table_list_t;



/* ---------------------------------------------------------------
 *
 * @class: table_man_t
 *
 * @brief: Base class for operations on a Shore table. 
 *
 * --------------------------------------------------------------- */

class table_man_t
{
public:

    table_man_t() {}

    virtual ~table_man_t() {}


    /* ------------------------------------------ */
    /* --- populate table with data from file --- */
    /* ------------------------------------------ */

    virtual w_rc_t load_from_file(ss_m* db, const char* fname = NULL)=0;


    /* ------------------------------------ */
    /* --- bulkload the specified index --- */
    /* ------------------------------------ */

    virtual w_rc_t bulkload_all_indexes(ss_m* db)=0;
    virtual w_rc_t bulkload_index(ss_m* db, const char* name)=0;
    virtual w_rc_t bulkload_index(ss_m* db, index_desc_t* pindex)=0;
    virtual w_rc_t bulkload_index_with_iterations(ss_m* db, index_desc_t* pindex)=0;
    virtual w_rc_t bulkload_index_with_helper(ss_m* db, index_desc_t* pindex)=0;


    /* ------------------------------------------------------- */
    /* --- check consistency between the indexes and table --- */
    /* ------------------------------------------------------- */

    /**  true:  consistent
     *   false: inconsistent */
    virtual w_rc_t check_all_indexes_together(ss_m* db)=0;
    virtual bool   check_all_indexes(ss_m* db)=0;
    virtual w_rc_t check_index(ss_m* db, index_desc_t* pidx)=0;
    virtual w_rc_t scan_all_indexes(ss_m* db)=0;
    virtual w_rc_t scan_index(ss_m* db, index_desc_t* pidx)=0;


    /* ----------------- */
    /* --- debugging --- */
    /* ----------------- */

    virtual w_rc_t print_table(ss_m* db)=0; /* print the table on screen */


}; // EOF: table_man_t




/****************************************************************** 
 *
 *  class table_desc_t methods 
 *
 ******************************************************************/



/****************************************************************** 
 *
 * @fn:    find_field_by_name
 *
 * @brief: Returns the field index, given its name. If no such field 
 *         name exists it returns -1.
 *
 ******************************************************************/

inline int table_desc_t::find_field_by_name(const char* field_name) const
{
    for (int i=0; i<_field_count; i++) {
        if (strcmp(field_name, _desc[i].name())==0) 
            return (i);
    }
    return (-1);
}



/****************************************************************** 
 *
 * @fn:    index_keydesc
 *
 * @brief: Iterates over all the fields of a selected index and returns 
 *         on a single string the corresponding key description
 *
 ******************************************************************/

inline const char* table_desc_t::index_keydesc(index_desc_t* idx)
{
    CRITICAL_SECTION(idx_kd_cs, idx->_keydesc_lock);
    if (strlen(idx->_keydesc)>1) // is key_desc is already set
        return (idx->_keydesc);

    // else set the index keydesc
    for (int i=0; i<idx->field_count(); i++) {
        strcat(idx->_keydesc, _desc[idx->key_index(i)].keydesc());
    }
    return (idx->_keydesc);
}



/******************************************************************
 *
 *  @fn:    index_maxkeysize
 *
 *  @brief: For an index it returns the maximum size of the index key
 *
 *  @note:  !!! Now that key_size() Uses the maxsize() of each field, 
 *              key_size() == maxkeysize()
 *
 ******************************************************************/

inline const int table_desc_t::index_maxkeysize(index_desc_t* idx) const
{
    register uint_t size = 0;
    if ((size = idx->get_keysize()) > 0) {
        // keysize has already been calculated
        // just return that value
        return (size);
    }
    
    // needs to calculate the (max)key for that index
    register uint_t ix = 0;
    for (uint_t i=0; i<idx->field_count(); i++) {
        ix = idx->key_index(i);
        size += _desc[ix].fieldmaxsize();
    }    
    // set it for the index, for future invokations
    idx->set_keysize(size);
    return(size);
}



/****************************************************************** 
 *
 *  @fn:    maxsize()
 *
 *  @brief: Return the maximum size requirement for a tuple in disk format.
 *          Normally, it will be calculated only once.
 *
 ******************************************************************/

inline const int table_desc_t::maxsize()
{
    // shortcut not to re-compute maxsize
    if (*&_maxsize)
        return (*&_maxsize);

    // calculate maximum size required   
    uint_t size = 0;
    uint_t var_count = 0;
    uint_t null_count = 0;
    for (int i=0; i<_field_count; i++) {
        size += _desc[i].fieldmaxsize();
        if (_desc[i].allow_null()) null_count++;
        if (_desc[i].is_variable_length()) var_count++;
    }
    
    size += (var_count*sizeof(offset_t)) + (null_count>>3) + 1;

    // There is a small window from the time it checks if maxsize is already set,
    // until the time it tries to set it up. In the meantime, another thread may
    // has done the calculation already. If that had happened, the two threads 
    // should have calculated the same maxsize, since it is the same table desc.
    // In other words, the maxsize should be either 0 or equal to the size.
    assert ((*&_maxsize==0) || (*&_maxsize==size));

    atomic_swap_uint(&_maxsize, size);
    return (*&_maxsize);
}


EXIT_NAMESPACE(shore);

#endif /* __SHORE_TABLE_H */
