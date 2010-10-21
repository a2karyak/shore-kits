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

/** @file shore_table.cpp
 *
 *  @brief Implementation of shore_table class
 *
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#include "sm/shore/shore_table.h"

using namespace shore;


/****************************************************************** 
 *
 *  class table_desc_t methods 
 *
 ******************************************************************/


/* ----------------------------------------- */
/* --- create physical table and indexes --- */
/* ----------------------------------------- */


table_desc_t::table_desc_t(const char* name, int fieldcnt)
    : file_desc_t(name, fieldcnt), 
      _indexes(NULL), 
      _primary_idx(NULL),
      _maxsize(0)
{
    // Create placeholders for the field descriptors
    _desc = new field_desc_t[fieldcnt];
}
    

table_desc_t::~table_desc_t()
{
    if (_desc)
        delete [] _desc;

    if (_indexes)
        delete _indexes;
}



/********************************************************************* 
 *
 *  @fn:    create_table
 *
 *  @brief: Creates the physical table and all the corresponding indexes
 *
 *********************************************************************/

w_rc_t table_desc_t::create_table(ss_m* db)
{
    if (!is_vid_valid() || !is_root_valid())
	W_DO(find_root_iid(db));

    // create the table
    W_DO(db->create_file(vid(), _fid, smlevel_3::t_regular));

    // add table entry to the metadata tree
    file_info_t file;
    file.set_ftype(FT_HEAP);
    file.set_fid(_fid);
    W_DO(ss_m::create_assoc(root_iid(),
			    vec_t(name(), strlen(name())),
			    vec_t(&file, sizeof(file_info_t))));
    

    // create all the indexes of the table
    index_desc_t* index = _indexes;
    stid_t iid = stid_t::null;
    ss_m::ndx_t smidx_type = ss_m::t_uni_btree;
    ss_m::concurrency_t smidx_cc = ss_m::t_cc_im;
    
    while (index) {

        TRACE( TRACE_ALWAYS, "IDX (%s) (%s) (%s) (%s) (%s)\n",
               index->name(), 
               (index->is_mr() ? "mrbt" : "bt"),
               (index->is_partitioned() ? "part" : "no part"), 
               (index->is_unique() ? "unique" : "no unique"),
               (index->is_relaxed() ? "relaxed" : "no relaxed"));

        // the type of index to create
        if (index->is_mr()) {
            // MRBTree
            smidx_type = index->is_unique() ? ss_m::t_uni_mrbtree : ss_m::t_mrbtree;
        }
        else {
            // Regular BTree
            smidx_type = index->is_unique() ? ss_m::t_uni_btree : ss_m::t_btree;
        }

        // what kind of CC will be used
        smidx_cc = index->is_relaxed() ? ss_m::t_cc_none : ss_m::t_cc_im;

        // if it is the primary, update file flag
        if (index->is_primary()) {
            file.set_ftype(FT_PRIMARY_IDX);
        }
        else {
            file.set_ftype(FT_IDX);
        }


        // create one index or multiple, if the index is partitioned
	if(index->is_partitioned()) {
	    for(int i=0; i < index->get_partition_count(); i++) {
                if (!index->is_mr()) {
                    W_DO(db->create_index(_vid, smidx_type, ss_m::t_regular,
                                          index_keydesc(index), smidx_cc, iid));
                }
                else {
                    W_DO(db->create_mr_index(_vid, smidx_type, ss_m::t_regular,
                                             index_keydesc(index), smidx_cc, iid,
                                             index->is_latchless()));
                }
		index->set_fid(i, iid);
		
		// add index entry to the metadata tree		
		file.set_fid(iid);
		char tmp[100];
		sprintf(tmp, "%s_%d", index->name(), i);
		W_DO(db->create_assoc(root_iid(),
				      vec_t(tmp, strlen(tmp)),
				      vec_t(&file, sizeof(file_info_t))));
	    }
	}
	else {
            if (!index->is_mr()) {
                W_DO(db->create_index(_vid, smidx_type, ss_m::t_regular,
                                      index_keydesc(index), smidx_cc, iid));
            }
            else {
                W_DO(db->create_mr_index(_vid, smidx_type, ss_m::t_regular,
                                         index_keydesc(index), smidx_cc, iid,
                                         index->is_latchless()));

                // If we already know the partitioning set it up
                W_DO(db->make_equal_partitions(iid,_minKey,_maxKey,_numParts));
            }                
	    index->set_fid(0, iid);

	    // add index entry to the metadata tree
	    file.set_fid(iid);
	    W_DO(db->create_assoc(root_iid(),
				  vec_t(index->name(), strlen(index->name())),
				  vec_t(&file, sizeof(file_info_t))));
	}
	
        // move to the next index of the table
	index = index->next();
    }
    
    return (RCOK);
}


/****************************************************************** 
 *  
 *  @fn:    create_index
 *
 *  @brief: Create a regular or primary index on the table
 *
 *  @note:  This only creates the index decription for the index in memory. 
 *
 ******************************************************************/


#warning Cannot update fields included at indexes - delete and insert again

#warning Only the last field of an index can be of variable length

bool table_desc_t::create_index(const char* name,
                                int partitions,
                                const uint* fields,
                                const uint num,
                                const bool unique,
                                const bool primary,
                                const uint4_t& pd)
{
    index_desc_t* p_index = new index_desc_t(name, num, partitions, fields, 
                                             unique, primary, pd);

    // check the validity of the index
    for (uint_t i=0; i<num; i++)  {
        assert(fields[i] < _field_count);

        // only the last field in the index can be variable lengthed
#warning IP: I am not sure if still only the last field in the index can be variable lengthed

        if (_desc[fields[i]].is_variable_length() && i != num-1) {
            assert(false);
        }
    }

    // link it to the list
    if (_indexes == NULL) _indexes = p_index;
    else _indexes->insert(p_index);

    // add as primary
    if (p_index->is_unique() && p_index->is_primary())
        _primary_idx = p_index;

    return true;
}


bool table_desc_t::create_primary_idx(const char* name,
                                      int partitions,
                                      const uint* fields,
                                      const uint num,
                                      const uint4_t& pd)
{
    index_desc_t* p_index = new index_desc_t(name, num, partitions, fields, 
                                             true, true, pd);

    // check the validity of the index
    for (uint_t i=0; i<num; i++) {
        assert(fields[i] < _field_count);

        // only the last field in the index can be variable lengthed
        if (_desc[fields[i]].is_variable_length() && i != num-1) {
            assert(false);
        }
    }

    // link it to the list of indexes
    if (_indexes == NULL) _indexes = p_index;
    else _indexes->insert(p_index);

    // make it the primary index
    _primary_idx = p_index;

    return (true);
}


/* ---------------------------------------------------- */
/* --- partitioning information, used with MRBTrees --- */
/* ---------------------------------------------------- */

w_rc_t table_desc_t::set_partitioning(const vec_t& minKey, 
                                      const vec_t& maxKey, 
                                      uint numParts)
{
    _minKey.set(minKey);
    _maxKey.set(maxKey);
    _numParts = numParts;
    return (RCOK);
}


/* ----------------- */
/* --- debugging --- */
/* ----------------- */


// For debug use only: print the description for all the field
void table_desc_t::print_desc(ostream& os)
{
    os << "Schema for table " << _name << endl;
    os << "Numer of fields: " << _field_count << endl;
    for (uint_t i=0; i<_field_count; i++) {
	_desc[i].print_desc(os);
    }
}


// #include <strstream>
// char const* db_pretty_print(table_desc_t const* ptdesc, int /* i=0 */, char const* /* s=0 */) 
// {
//     static char data[1024];
//     std::strstream inout(data, sizeof(data));
//     ((table_desc_t*)ptdesc)->print_desc(inout);
//     inout << std::ends;
//     return data;
// }

#include <sstream>
char const* db_pretty_print(table_desc_t const* ptdesc, int /* i=0 */, char const* /* s=0 */) 
{
    static char data[1024];
    std::stringstream inout(data, stringstream::in | stringstream::out);
    //std::strstream inout(data, sizeof(data));
    ((table_desc_t*)ptdesc)->print_desc(inout);
    inout << std::ends;
    return data;
}


