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

/** @file:   range_part_table.cpp
 *
 *  @brief:  Range partitioned table class in DORA
 *
 *  @date:   Aug 2010
 *
 *  @author: Ippokratis Pandis (ipandis)
 */

#include "dora/range_part_table.h"
#include "dora/dora_error.h"

using namespace shore;


ENTER_NAMESPACE(dora);


/****************************************************************** 
 *
 * @fn:    constructor
 *
 * @brief: Populates the key-range map and creates one partition per
 *
 ******************************************************************/

range_table_t::range_table_t(ShoreEnv* env, table_desc_t* ptable,
                             const processorid_t aprs,
                             const uint acpurange,
                             const uint keyEstimation,
                             const cvec_t& minKey,
                             const cvec_t& maxKey,
                             const uint pnum) 
    : part_table_t(env,ptable,aprs,acpurange,keyEstimation,minKey,maxKey,pnum)
{
    fprintf(stdout, "Creating (%d) (%s) parts\n", _pcnt, _table->name());

    // Create the key-range map   
    _prMap = new dkey_ranges_map(PartTable::_min,PartTable::_max,pnum);

    // Setup partitions based on the boundaries created in the key-range map 
    typedef vector< pair<char*,char*> >           boundariesVector;
    typedef vector< pair<char*,char*> >::iterator boundariesVectorIter;
    boundariesVector bvec;
    w_rc_t r = _prMap->getBoundariesVec(bvec);
    if (r.is_error()) {
        fprintf( stdout, "Error getting partition boundaries\n");
        assert (0);
    }

    uint idx=0;
    for (boundariesVectorIter it = bvec.begin(); it != bvec.end(); it++,++idx) {
        char* pdown = (*it).first;
        char* pup = (*it).second;
        cvec_t cvdown(pdown,strlen(pdown));
        cvec_t cvup(pup,strlen(pup));
        r = create_one_part(idx,cvdown,cvup);
        if (r.is_error()) {
            fprintf( stdout, "Error creating partition (%s-%d)\n", 
                     _table->name(),idx);
            assert (0);
        }
    }
}


range_table_t::~range_table_t()
{
}


/****************************************************************** 
 *
 * @fn:    create_one_part()
 *
 * @brief: Creates one partition and adds it to the vector
 *
 ******************************************************************/

w_rc_t range_table_t::create_one_part(const uint idx, const cvec_t& down, const cvec_t& up)
{   
    CRITICAL_SECTION(conf_cs, PartTable::_pcgf_lock);

    // Create a new partition object
    base_partition_t* pbp = _create_one_part(idx,down,up);

    if (!pbp) {
        TRACE( TRACE_ALWAYS, "Problem in creating partition (%d) for (%s)\n", 
               PartTable::_pcnt, PartTable::_table->name());
        assert (0); // should not happen
        return (RC(de_GEN_PARTITION));
    }
    assert (pbp);
    PartTable::_ppvec.push_back(pbp);

    // Update next cpu
    {
        CRITICAL_SECTION(next_prs_cs, PartTable::_next_prs_lock);
        PartTable::_next_prs_id = PartTable::next_cpu(PartTable::_next_prs_id);
    }

    ++PartTable::_pcnt;
    return (RCOK);
}


EXIT_NAMESPACE(dora);

