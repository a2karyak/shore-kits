/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file range_part_table.h
 *
 *  @brief Range partitioned table class in DORA
 *
 *  @author Ippokratis Pandis, Oct 2008
 */


#ifndef __DORA_RANGE_PART_TABLE_H
#define __DORA_RANGE_PART_TABLE_H


#include "dora.h"


ENTER_NAMESPACE(dora);


using namespace shore;


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

    typedef  range_partition_impl<DataType> rp_impl;

private:

    // range-partition field count
    int _field_count;

public:

    range_part_table_impl(ShoreEnv* env, table_desc_t* ptable,
                          const processorid_t aprs,
                          const int arange,
                          const int field_count,
                          const int apcnt = DF_NUM_OF_PARTITIONS_PER_TABLE) 
        : part_table_t(env, ptable, aprs, arange, apcnt), 
          _field_count(field_count)
    {
        assert (_field_count>0);
    }

    ~range_part_table_impl() { }    

    const int create_one_part();

}; // EOF: range_part_table_impl


/****************************************************************** 
 *
 * @fn:    create_one_part()
 *
 * @brief: Creates one partition and adds it to the vector
 *
 ******************************************************************/

template <class DataType>
inline const int range_part_table_impl<DataType>::create_one_part()
{
    // 1. a new partition object
    rp_impl* aprpi = new rp_impl(_env, _table, _field_count, _pcnt, _next_prs_id);
    if (!aprpi) {
        TRACE( TRACE_ALWAYS, "Problem in creating partition (%d) for (%s)\n", 
               _pcnt, _table->name());
        assert (0); // should not happen
        return (de_GEN_PARTITION);
    }
    assert (aprpi);

    // 2. add to vector
    // 3. increase counter of partitions
    {
        CRITICAL_SECTION(conf_cs, _pcgf_lock);
        _ppvec.push_back(aprpi);
        ++_pcnt;        
    }

    // 5. update next cpu
    {
        CRITICAL_SECTION(next_prs_cs, _next_prs_lock);
        _next_prs_id = next_cpu(_next_prs_id);
    }
    return (0);
}



EXIT_NAMESPACE(dora);

#endif /** __DORA_RANGE_PART_TABLE_H */

