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

/** @file:   dkey_ranges_map.h
 *
 *  @brief:  Definition of a map of key ranges to partitions used by
 *           DORA MRBTrees.
 *
 *  @notes:  The keys are Shore-mt cvec_t. Thread-safe.  
 *
 *  @date:   Aug 2010
 *
 *  @author: Pinar Tozun (pinar)
 *  @author: Ippokratis Pandis (ipandis)
 *  @author: Ryan Johnson (ryanjohn)
 */


#ifndef __DORA_DKEY_RANGES_MAP_H
#define __DORA_DKEY_RANGES_MAP_H

#include "util/key_ranges_map.h"

using namespace std;

ENTER_NAMESPACE(dora);


/******************************************************************** 
 *
 * @class: dkey_ranges_map
 *
 * @brief: A map of key ranges to DORA partitions. This structure is used
 *         by the DORA version of the multi-rooted B-tree (DMRBTree). 
 *
 * @note:  The specific implementation indentifies each partition through
 *         a partition id (uint). Hence, this implementation is for DORA 
 *         MRBTrees.
 *
 ********************************************************************/

class dkey_ranges_map : public key_ranges_map
{
private:    

    typedef vector<lpid_t>              lpidVec;
    typedef vector<lpid_t>::iterator    lpidVecIter;

    typedef map<lpid_t, uint>           lpidPartMap;
    typedef map<lpid_t, uint>::iterator lpidPartMapIter;

    typedef map<uint, lpid_t>           partLpidMap;
    typedef map<uint, lpid_t>::iterator partLpidMapIter;

    typedef cvec_t Key;

    // A simple birectional association of lpid_t to uint. 
    // In the baseline key_ranges_map each sub-tree (partition) is identified
    // by the lpid_t of the root of the corresponding sub-tree. 
    // On the other hand, DORA needs a way to point to a partition. We use a
    // simple uint which is the partition number.
    lpidPartMap _lpidPartMap;
    partLpidMap _partLpidMap;

    occ_rwlock _bimapLock;

    // A watermark for assigning unique partition numbers
    volatile uint _maxpnum;

public:
  
    dkey_ranges_map(const Key& minKey, const Key& maxKey, const uint numParts);
    ~dkey_ranges_map();

    // Splits the partition where "key" belongs to two partitions. The start of 
    // the second partition is the "key".
    w_rc_t addPartition(const Key& key, uint& pnum);

    // Deletes the given partition (identified by the partition number), by 
    // merging it with the previous partition
    w_rc_t deletePartition(const uint pnum);

    // Gets the partition id of the given key.
    //
    // @note: In the DORA version each partition is identified by a partition-id 
    w_rc_t getPartitionByKey(const Key& key, uint& pnum);
    w_rc_t operator()(const Key& key, uint& pnum);

    // Returns the list of partitions that cover: 
    // [key1, key2], (key1, key2], [key1, key2), or (key1, key2) ranges
    w_rc_t getPartitions(const Key& key1, bool key1Included, 
                         const Key& key2, bool key2Included,                         
                         vector<uint>& pnumVec);

    // Returns the range boundaries of a partition
    w_rc_t getBoundaries(uint pnum, pair<cvec_t, cvec_t>& keyRange);
    
}; // EOF: dkey_ranges_map

EXIT_NAMESPACE(dora);

#endif // __DORA_DKEY_RANGES_MAP_H
