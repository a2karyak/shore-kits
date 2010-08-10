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

/** @file:   dkey_ranges_map.cpp
 *
 *  @brief:  Implementation of a map of key ranges to partitions used by
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


#include "dora/dkey_ranges_map.h"


ENTER_NAMESPACE(dora);

dkey_ranges_map::dkey_ranges_map(const Key& minKey, const Key& maxKey, 
                                 uint numPartitions)
    : key_ranges_map(minKey,maxKey,numPartitions), _maxpnum(0)
{
}

dkey_ranges_map::~dkey_ranges_map()
{
}



/****************************************************************** 
 *
 * @fn:    addPartition()
 *
 * @brief: Splits the partition where "key" belongs to two partitions. 
 *         The start of the second partition is the "key".
 *
 * @note:  Updates also the lpid->partNum map
 *
 ******************************************************************/

w_rc_t dkey_ranges_map::addPartition(const Key& key, uint& pnum)
{
    // Add a partition
    lpid_t apid;
    W_DO(key_ranges_map::addPartition(key,apid));

    // If successful, update the lpid->partNum associations
    _rwlock.acquire_write();
    uint nv = _maxpnum++;
    _lpidPartMap[apid] = nv;
    _partLpidMap[nv] = apid;
    _rwlock.release_write();

    pnum = nv;
    
    return (RCOK);
}


/****************************************************************** 
 *
 * @fn:    deletePartition()
 *
 * @brief: Deletes a partition based on the partition number.
 *
 * @note:  The deletion is achieved by merging the partition with the
 *         partition which is before that, unless the partition is 
 *         the left-most partition.
 *
 ******************************************************************/

w_rc_t dkey_ranges_map::deletePartition(const uint aPartNum)
{
    w_rc_t r = RCOK;

    // Get the lpid_t of the partition to be deleted
    _bimapLock.acquire_write();
    partLpidMapIter it = _partLpidMap.find(aPartNum);
    if (it == _partLpidMap.end()) {
        // There is not partition with such number
        _bimapLock.release_write();
        return (RC(mrb_PARTITION_NOT_FOUND));
    }

    // Delete the partition
    lpid_t pid = (*it).second;
    r = key_ranges_map::deletePartition(pid);
    
    // If successful, remove the delete partition entry from the two maps 
    if (!r.is_error()) {
        _lpidPartMap.erase(pid);
        _partLpidMap.erase(aPartNum);
    }
    _bimapLock.release_write();
    return (r);
}


/****************************************************************** 
 *
 * @fn:    getPartitionByKey()
 *
 * @brief: Returns the partition which a particular key belongs to
 *
 ******************************************************************/

w_rc_t dkey_ranges_map::getPartitionByKey(const Key& key, uint& pnum)
{
    w_rc_t r = RCOK;

    lpid_t pid;
    r = key_ranges_map::getPartitionByKey(key,pid);

    if (!r.is_error()) {
        _bimapLock.acquire_read();
        lpidPartMapIter it = _lpidPartMap.find(pid);
        if (it != _lpidPartMap.end()) {
            pnum = (*it).second;
        }
        else {
            // No such association exists
            r = RC(mrb_PARTITION_NOT_FOUND);
        }
        _bimapLock.release_read();
    }
    return (r);
}

w_rc_t dkey_ranges_map::operator()(const Key& key, uint& pnum)
{
    return (getPartitionByKey(key,pnum));
}



/****************************************************************** 
 *
 * @fn:    getPartitions()
 *
 * @brief: Returns the list of partitions that cover one of the key ranges:
 *         [key1, key2], (key1, key2], [key1, key2), or (key1, key2) 
 *
 ******************************************************************/

w_rc_t dkey_ranges_map::getPartitions(const Key& key1, bool key1Included, 
                                     const Key& key2, bool key2Included,
                                     vector<uint>& pnumVec) 
{
    lpidVec vecPid;
    W_DO(key_ranges_map::getPartitions(key1,key1Included,key2,key2Included,vecPid));

    // Iterate over all the return lpid_t and add the to return vector
    // the corresponding pnum.
    if (!vecPid.empty()) {
        lpidPartMapIter it;
        _bimapLock.acquire_read();
        for (lpidVecIter vecit = vecPid.begin(); vecit != vecPid.end(); ++vecit) {
            it = _lpidPartMap.find(*vecit);
            if (it == _lpidPartMap.end()) {
                // It better be there
                _bimapLock.release_read();
                return (RC(mrb_PARTITION_NOT_FOUND));
            }
            pnumVec.push_back((*it).second);
        }
        _bimapLock.release_read();
    }
    return (RCOK);
}


/****************************************************************** 
 *
 * @fn:    getBoundaries()
 *
 * @brief: Returns the range boundaries of a partition, identified by
 *         its partition number
 *
 ******************************************************************/

w_rc_t dkey_ranges_map::getBoundaries(uint pnum, pair<cvec_t, cvec_t>& keyRange) 
{
    _bimapLock.acquire_read();
    partLpidMapIter it = _partLpidMap.find(pnum);
    if (it == _partLpidMap.end()) {
        // No such association exists
        _bimapLock.release_read();
        return (RC(mrb_PARTITION_NOT_FOUND));
    }
    lpid_t pid = (*it).second;
    _bimapLock.release_read();
    return (key_ranges_map::getBoundaries(pid,keyRange));
}


EXIT_NAMESPACE(dora);
