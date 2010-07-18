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

/** @file:   range_part_table.h
 *
 *  @brief:  Range partitioned table class in DORA
 *
 *  @author: Ippokratis Pandis, Oct 2008
 */


#ifndef __DORA_RANGE_PART_TABLE_H
#define __DORA_RANGE_PART_TABLE_H

#include "util/key_ranges_map.h"

#include "dora/part_table.h"
#include "dora/range_partition.h"


using namespace shore;


ENTER_NAMESPACE(dora);



/******************************************************************** 
 *
 * @class: range_par_table_impl
 *
 * @brief: Template-based class for a range data partitioned table
 *
 ********************************************************************/

template <class DataType>
class range_part_table_impl : public part_table_t< range_partition_impl<DataType> >
{
public:

    typedef part_table_t< range_partition_impl<DataType> > PartTable;
    typedef range_partition_impl<DataType> rpImpl;
    typedef key_wrapper_t<DataType>        Key;

private:

    // range-partition field count
    int _field_count;
    
public:

    range_part_table_impl(ShoreEnv* env, table_desc_t* ptable,
                          const processorid_t aprs,
                          const int arange,
                          const int field_count,
                          const int keyEstimation,
                          const int sfsperpart,
                          const int totalsf) 
        : PartTable(env, ptable, aprs, arange, keyEstimation, sfsperpart, totalsf), 
          _field_count(field_count)
    {
        assert (_field_count>0);


        // setup partitions
        Key partDown;
        Key partUp;
        int aboundary=0;

        // we are doing the partitioning based on the number of warehouses
        int parts_added = 0;

        for (int i=0; i<PartTable::_total_sf; i+=PartTable::_sfs_per_part) {
            create_one_part();
            partDown.reset();
            partDown.push_back(i);
            partUp.reset();
            aboundary=i+PartTable::_sfs_per_part;
            partUp.push_back(aboundary);
            PartTable::_ppvec[parts_added]->resize(partDown,partUp);
            ++parts_added;
        }
        TRACE( TRACE_DEBUG, "Table (%s) - (%d) partitions\n", 
               PartTable::_table->name(), parts_added);        
    }

    ~range_part_table_impl() { }    

    int create_one_part();

    inline rpImpl* myPart(const int asf) {
        //FIX ME FIX ME;
        // TODO: return (PartTable::_ppvec[_rangeMap(asf)]);
        return (PartTable::_ppvec[asf/PartTable::_sfs_per_part]);
    }

}; // EOF: range_part_table_impl


/****************************************************************** 
 *
 * @fn:    create_one_part()
 *
 * @brief: Creates one partition and adds it to the vector
 *
 ******************************************************************/

template <class DataType>
int range_part_table_impl<DataType>::create_one_part()
{   
    CRITICAL_SECTION(conf_cs, PartTable::_pcgf_lock);
    ++PartTable::_pcnt;        

    // 1. a new partition object
    rpImpl* aprpi = new rpImpl(PartTable::_env, PartTable::_table, _field_count, 
                               PartTable::_key_estimation, PartTable::_pcnt, 
                               PartTable::_next_prs_id);
    if (!aprpi) {
        TRACE( TRACE_ALWAYS, "Problem in creating partition (%d) for (%s)\n", 
               PartTable::_pcnt, PartTable::_table->name());
        assert (0); // should not happen
        return (de_GEN_PARTITION);
    }
    assert (aprpi);
    PartTable::_ppvec.push_back(aprpi);


    // 2. update next cpu
    {
        CRITICAL_SECTION(next_prs_cs, PartTable::_next_prs_lock);
        PartTable::_next_prs_id = PartTable::next_cpu(PartTable::_next_prs_id);
    }
    return (0);
}


EXIT_NAMESPACE(dora);

#endif /** __DORA_RANGE_PART_TABLE_H */

