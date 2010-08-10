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

/** @file:   range_table_i.h
 *
 *  @brief:  Template-based implementation of range-partitioned tables in DORA
 *
 *  @date:   Aug 2010
 *
 *  @author: Ippokratis Pandis (ipandis)
 */


#ifndef __DORA_RANGE_TABLE_I_H
#define __DORA_RANGE_TABLE_I_H

#include "dora/range_part_table.h"
#include "dora/range_partition.h"

using namespace shore;


ENTER_NAMESPACE(dora);


/******************************************************************** 
 *
 * @class: range_table_i
 *
 * @brief: Template-based class for a range data partitioned table
 *
 ********************************************************************/

template <class DataType>
class range_table_i : public range_table_t
{
public:

    typedef key_wrapper_t<DataType>     Key;
    typedef action_t<DataType>          Action;
    typedef range_partition_i<DataType> rpImpl;

public:

    range_table_i(ShoreEnv* env, table_desc_t* ptable,
                  const processorid_t aprs,  
                  const uint acpurange,
                  const uint keyEstimation,
                  const cvec_t& minKey,   
                  const cvec_t& maxKey,
                  const uint pnum)
        : range_table_t(env,ptable,aprs,acpurange,keyEstimation,minKey,maxKey,pnum)
    { }

    ~range_table_i() { }

    // Return a pointer to the partition implementation responsible for a 
    // specific DORA key.
    // This is the function that does the translation of a DORA key to a cvec_t
    inline rpImpl* getPartByDKey(const Key& key) {
        return (_getPartByKey(key.toCVec()));
    }

    // Return a pointer to the partition implementation responsible for a 
    // specific cvec_t
    inline rpImpl* getPartByCVKey(const cvec_t& cvkey) {
        return (_getPartByKey(cvkey));
    }

    // Return a pointer to the partition based on the partition index
    inline rpImpl* getPartByIdx(const uint idx) {
        return ((rpImpl*)(PartTable::_ppvec[idx]));
    }

    // Enqueues action, false on error
    inline int enqueue(Action* paction, const bool bWake, const uint idx) {
        assert (idx<_pcnt);
        return (getPartByIdx(idx)->enqueue(paction,bWake));
    }

protected:

    base_partition_t* _create_one_part(const uint idx, 
                                       const cvec_t& down, 
                                       const cvec_t& up);


    // Return a pointer to the partition responsible for a specific key    
    inline rpImpl* _getPartByKey(const cvec_t& key) {
        uint idx=0;
        w_rc_t r = _prMap->getPartitionByKey(key,idx);
        if (r.is_error()) {
            return (NULL);
        }
        return (getPartByIdx(idx));
    }

}; // EOF: range_table_i


/****************************************************************** 
 *
 * @fn:    _create_one_part()
 *
 * @brief: Creates one partition (template-based)
 *
 ******************************************************************/

template <class DataType>
base_partition_t* range_table_i<DataType>::_create_one_part(const uint idx,
                                                            const cvec_t& down, 
                                                            const cvec_t& up)
{   
    return (new rpImpl(PartTable::_env, PartTable::_table,
                       PartTable::_key_estimation, 
                       idx, down, up,
                       PartTable::_next_prs_id));
}


EXIT_NAMESPACE(dora);

#endif /** __DORA_RANGE_TABLE_I_H */

