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

/** @file:   part_table.h
 *
 *  @brief:  Declaration of each table in DORA.
 *
 *  @note:   Implemented as a vector of partitions. The routing information 
 *           (for example, table in the case of range partitioning) is stored
 *           at the specific sub-class (for example, range_part_table_impl)
 *
 *  @author: Ippokratis Pandis, Oct 2008
 */


#ifndef __DORA_PART_TABLE_H
#define __DORA_PART_TABLE_H


#include <cstdio>

#include "util.h"

#include "sm/shore/shore_env.h"
#include "sm/shore/shore_table.h"

#include "dora/base_partition.h"

using namespace shore;


ENTER_NAMESPACE(dora);


/******************************************************************** 
 *
 * @class: part_table_t
 *
 * @brief: Abstract class for representing a table as a set of (data) partitions
 *
 ********************************************************************/

class part_table_t
{
public:

    typedef vector<base_partition_t*> PartitionPtrVector;
    typedef base_partition_t          Partition;

protected:

    ShoreEnv*          _env;    
    table_desc_t*      _table;

    // the vector of partitions
    PartitionPtrVector _ppvec;
    uint               _pcnt;
    tatas_lock         _pcgf_lock;

    // processor binding
    processorid_t      _start_prs_id;
    processorid_t      _next_prs_id;
    uint               _prs_range;
    tatas_lock         _next_prs_lock;

    // per partition key estimation
    uint               _key_estimation;

    // boundaries of possible key value, used for the enqueue function
    cvec_t _min;
    cvec_t _max;
   
public:

    part_table_t(ShoreEnv* env, table_desc_t* ptable,
                 const processorid_t aprs,
                 const uint acpurange,
                 const uint keyEstimation,
                 const cvec_t& minKey,
                 const cvec_t& maxKey,
                 const uint pnum);

    virtual ~part_table_t();


    // Access methods //
    PartitionPtrVector* get_vector();
    Partition* get_part(const uint pos);

    //// Control table ////

    // stops all partitions
    w_rc_t stop();

    // prepares all partitions for a new run
    w_rc_t prepareNewRun();

    // Return the appropriate partition. This decision is based on the type
    // of the partitioning scheme used
    virtual base_partition_t* getPartByKey(const cvec_t& key)=0;


    //// CPU placement ////

    // reset all partitions
    virtual w_rc_t reset();
    
    // move to another range of processors
    w_rc_t move(const processorid_t aprs, const uint acpurange);

    // decide the next processor
    virtual processorid_t next_cpu(const processorid_t& aprd);


    //// For debugging ////

    // information
    void statistics() const;

    // information
    void info() const;

    // dumps information
    void dump() const;

}; // EOF: part_table_t

EXIT_NAMESPACE(dora);

#endif /** __DORA_PART_TABLE_H */

