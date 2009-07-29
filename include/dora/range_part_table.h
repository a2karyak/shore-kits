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
        : part_table_t(env, ptable, aprs, arange, keyEstimation, sfsperpart, totalsf), 
          _field_count(field_count)
    {
        assert (_field_count>0);


        // setup partitions
        Key partDown;
        Key partUp;
        int aboundary=0;

        // we are doing the partitioning based on the number of warehouses
        int parts_added = 0;

        for (int i=0; i<_total_sf; i+=_sfs_per_part) {
            create_one_part();
            partDown.reset();
            partDown.push_back(i);
            partUp.reset();
            aboundary=i+_sfs_per_part;
            partUp.push_back(aboundary);
            _ppvec[parts_added]->resize(partDown,partUp);
            ++parts_added;
        }
        TRACE( TRACE_DEBUG, "Table (%s) - (%d) partitions\n", 
               _table->name(), parts_added);        
    }

    ~range_part_table_impl() { }    

    const int create_one_part();

    inline rpImpl* myPart(const int asf) {
        return (_ppvec[asf/_sfs_per_part]);
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
const int range_part_table_impl<DataType>::create_one_part()
{   
    CRITICAL_SECTION(conf_cs, _pcgf_lock);
    ++_pcnt;        

    // 1. a new partition object
    rpImpl* aprpi = new rpImpl(_env, _table, _field_count, _key_estimation, _pcnt, _next_prs_id);
    if (!aprpi) {
        TRACE( TRACE_ALWAYS, "Problem in creating partition (%d) for (%s)\n", 
               _pcnt, _table->name());
        assert (0); // should not happen
        return (de_GEN_PARTITION);
    }
    assert (aprpi);
    _ppvec.push_back(aprpi);


    // 2. update next cpu
    {
        CRITICAL_SECTION(next_prs_cs, _next_prs_lock);
        _next_prs_id = next_cpu(_next_prs_id);
    }
    return (0);
}



EXIT_NAMESPACE(dora);

#endif /** __DORA_RANGE_PART_TABLE_H */

