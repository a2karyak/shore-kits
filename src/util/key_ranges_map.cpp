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

/** @file:   key_ranges_map.cpp
 *
 *  @brief:  Implementation of a map of key ranges to partitions used by
 *           baseline MRBTrees.
 *
 *  @notes:  The keys are Shore-mt cvec_t. Thread-safe.  
 *
 *  @date:   July 2010
 *
 *  @author: Pinar Tozun (pinar)
 *  @author: Ippokratis Pandis (ipandis)
 *  @author: Ryan Johnson (ryanjohn)
 */


#include "util/key_ranges_map.h"
//#include "../../include/util/key_ranges_map.h"

/** key_ranges_map interface */

key_ranges_map::key_ranges_map(const Key& minKey, const Key& maxKey, uint numPartitions)
    : _numPartitions(numPartitions) 
{
    _minKey = (char*) malloc(minKey.size());
    _maxKey = (char*) malloc(maxKey.size());
    minKey.copy_to(_minKey);
    maxKey.copy_to(_maxKey);

    makeEqualPartitions();

    free (_minKey);
    free (_maxKey);
}


key_ranges_map::~key_ranges_map()
{
    // Delete the allocated keys
    keysIter iter;
    uint i=0;
    _rwlock.acquire_write();
    for (iter = _keyRangesMap.begin(); iter != _keyRangesMap.end(); ++iter, ++i) {
        TRACE( TRACE_DEBUG, "Partition %d\tStart (%s)\tEnd (%s)\n",
               i, iter->first, iter->second);
        free (iter->first);
        free (iter->second);
    }
    _rwlock.release_write();    
}

/*
inline void key_ranges_map::_getKey(const Key& key, char* keyS)
{
    //void* keyP = malloc(key.size());
    key.copy_to(keyS);
    //return (char*) keyP;
}
*/



/****************************************************************** 
 *
 * @fn:    makeEqualPartitions()
 *
 * @brief: ?
 *
 ******************************************************************/

void key_ranges_map::makeEqualPartitions()
{
    // TODO: discuss/think about what to do here
    assert (0);

    /*
      int range = (_maxKey - _minKey) / _numPartitions;
      int currentLower = 0;
      int currentUpper = _minKey;
      _rwlock.acquire_write();
      _keyRangesMap.clear();
      for (int i = 0; i < _numPartitions; i++) {
      currentLower = currentUpper;
      currentUpper = currentLower + range;
      _keyRangesMap[currentLower] = currentUpper;
      }
      _keyRangesMap[currentLower] = _maxKey + 1;
      _rwlock.release_write();
    */

}


/****************************************************************** 
 *
 * @fn:    addPartition()
 *
 * @brief: Splits the partition where "key" belongs to two partitions. 
 *         The start of the second partition is the "key".
 *
 ******************************************************************/

w_rc_t key_ranges_map::addPartition(const Key& key)
{
    w_rc_t r = RCOK;

    char* keyS = (char*) malloc(key.size());
    key.copy_to(keyS);
    _rwlock.acquire_write();
    keysIter iter = _keyRangesMap.lower_bound(keyS);
    if (iter != _keyRangesMap.end()) {
        _keyRangesMap[keyS] = iter->second;
        _keyRangesMap[iter->first] = keyS;
        _numPartitions++;
    }
    else {
        r = RC(mrb_PARTITION_NOT_FOUND);
    }
    _rwlock.release_write();
    return (r);
}


/****************************************************************** 
 *
 * @fn:    deletePartition{,ByKey}()
 *
 * @brief: Deletes a partition, by merging it with the partition which
 *         is before that, based either on a partition identified or
 *         a key.
 *
 ******************************************************************/

w_rc_t key_ranges_map::_deletePartitionByKey(char* key)
{
#warning IP: Pinar what happens if it is the left-most partition?
    w_rc_t r = RCOK;

    _rwlock.acquire_write();
    keysIter iter = _keyRangesMap.lower_bound(key);
#warning IP: Pinar what happens if it does not find a partition?

    ++iter;
    if(iter == _keyRangesMap.end()) {
        // If the given key is in the last partition
        --iter; 
    }
    _keyRangesMap[iter->first] = _keyRangesMap[iter->second];
    --iter;
    _keyRangesMap.erase(iter);
    _numPartitions--;
    _rwlock.release_write();
    return (r);
}

w_rc_t key_ranges_map::deletePartitionByKey(const Key& key)
{
    w_rc_t r = RCOK;
    char* keyS = (char*) malloc(key.size());
    key.copy_to(keyS);
    r = _deletePartitionWithKey(keyS);
    free (keyS);
    return (r);
}

w_rc_t key_ranges_map::deletePartition(lpid_t pid)
{
    w_rc_t r = RCOK;

#warning IP: This one needs rewriting
    keysIter iter;
    uint i;
    _rwlock.acquire_read();
    for (i = 0, iter = _keyRangesMap.begin(); iter != _keyRangesMap.end() && i < partition; ++iter, i++)
        ;
    _rwlock.release_read();
    if (iter != _keyRangesMap.end()) {
        r = _deletePartitionWithKey(iter->first);
    }

    return (r);
}


/****************************************************************** 
 *
 * @fn:    getPartitionByKey()
 *
 * @brief: Returns the partition which a particular key belongs to
 *
 ******************************************************************/

w_rc_t key_ranges_map::getPartitionByKey(const Key& key, lpid_t& pid)
{
    char* keyS = (char*) malloc(key.size());
    key.copy_to(keyS);
    // TODO: if this just wants the init_key as the return value
    // you can easily change it to
    // keysIter iter = _keyRangesMap.lower_bound(key);
    // return iter->first;
    keysIter iter;
    uint partitionNum = 0;
    _rwlock.acquire_read();
    for (iter = _keyRangesMap.begin(); iter != _keyRangesMap.end(); ++iter) {
        if (strcmp(keyS, iter->first) >= 0  && strcmp(keyS, iter->second) < 0) {
            _rwlock.release_read();
	    free (keyS);
            pid = partitionNum;
            return (RCOK); // found
        }
        partitionNum++;
    }
    // If reached this point the key is not in the map, returns error.
    free (keyS);
    return (RC(mrb_PARTITION_NOT_FOUND));
}

w_rc_t key_ranges_map::operator()(const Key& key, lpid_t& pid)
{
    return (getPartitionWithKey(key,pid));
}



/****************************************************************** 
 *
 * @fn:    getPartitions()
 *
 * @brief: Returns the list of partitions that cover one of the key ranges:
 *         [key1, key2], (key1, key2], [key1, key2), or (key1, key2) 
 *
 ******************************************************************/

w_rc_t key_ranges_map::getPartitions(const Key& key1, bool key1Included, 
                                     const Key& key2, bool key2Included,
                                     vector<lpid_t>& pidVec) 
{
    w_rc_t r = RCOK;
  
#warning IP: This needs rewrite
    vector<uint> partitions;
    // find the partition key1 is in
    uint partition1 = getPartitionWithKey(key1);
    // TODO: actually you're doing redundant work here, if key1<key2 then you can continue the iteration from where key2 is left off
    // find the partition key2 is in
    uint partition2 = getPartitionWithKey(key2);
    // TODO: check corner cases
    /*
      if(!key1Included) {
      keysIter iter1 = _keyRangesMap.lower_bound(key1);
      if(iter1->second == key1+1)
      partition1--;
      }
      if(!key2Included) {
      keysIter iter2 = _keyRangesMap.lower_bound(key2);
      if(iter2->first == key2)
      partition2++;
      }
    */
    for(uint i = partition2; i<=partition1; i++) {
        partitions.push_back(i);
    }

    pidVec = partitions;

    return (r);
}


/****************************************************************** 
 *
 * @fn:    getBoundaries()
 *
 * @brief: Returns the range boundaries of a partition
 *
 ******************************************************************/

w_rc_t key_ranges_map::getBoundaries(uint partition, pair<cvec_t, cvec_t>& keyRange) 
{
    keysIter iter;
    uint i;
    _rwlock.acquire_read();
    for (i = 0, iter = _keyRangesMap.begin(); iter != _keyRangesMap.end() && i< partition; ++iter, i++)
        ;
    _rwlock.release_read();
    // TODO: Not sure whether this is correct, should check
    keyRange.first.put(iter->first, sizeof(iter->first));
    keyRange.second.put(iter->second, sizeof(iter->second));

    return (RCOK);
}



/****************************************************************** 
 *
 * Helper functions
 *
 ******************************************************************/

void key_ranges_map::printPartitions()
{
    keysIter iter;
    uint i = 0;
    _rwlock.acquire_read();
    for (iter = _keyRangesMap.begin(); iter != _keyRangesMap.end(); ++iter, i++) {
        TRACE( TRACE_DEBUG, "Partition %d\tStart (%s)\tEnd (%s)\n",
               i, iter->first, iter->second);
    }
    _rwlock.release_read();
}

void key_ranges_map::setNumPartitions(uint numPartitions)
{
    _rwlock.acquire_write();
    _numPartitions = numPartitions;
    // TODO: update partitions
    _rwlock.release_write();
}

void key_ranges_map::setMinKey(const Key& minKey)
{  
    _rwlock.acquire_write();
    minKey.copy_to(_minKey);
    keysIter iter = _keyRangesMap.end();
    iter--;
    _keyRangesMap[_minKey] = iter->second;
    _keyRangesMap.erase(iter);
    _rwlock.release_write();
}

void key_ranges_map::setMaxKey(const Key& maxKey)
{
    _rwlock.acquire_write();
    maxKey.copy_to(_maxKey);
    keysIter iter = _keyRangesMap.begin();
    // TODO: here the iter->second is included in the range, so this should be fixed
    iter->second = _maxKey;
    _rwlock.release_write();
}

int main(void)
{
    // TODO: update the test
    /*
      cout << "key_ranges_map(10, 100, 10)" << endl;
      key_ranges_map* KeyMap = new key_ranges_map(10, 100, 10);
      (*KeyMap).printPartitions();
      cout << endl;
  
      cout << "addPartition(60)" << endl;
      (*KeyMap).addPartition(60);
      (*KeyMap).printPartitions();
      cout << endl;
  
      cout << "deletePartitionWithKey(20)" << endl;
      (*KeyMap).deletePartitionWithKey(20);
      (*KeyMap).printPartitions();
      cout << endl;
  
      cout << "deletePartition(9)" << endl;
      (*KeyMap).deletePartition(9);
      (*KeyMap).printPartitions();
      cout << endl;
  
      cout << "getPartitionWithKey(70)" << endl;
      cout << (*KeyMap).getPartitionWithKey(70) << endl;
      cout << endl;
  
      cout << (*KeyMap)(70) << endl;
      cout << endl;
  
      cout << "setMinKey(6)" << endl;
      (*KeyMap).setMinKey(6);
      (*KeyMap).printPartitions();
      cout << endl;
  
      cout << "setMaxKey(110)" << endl;
      (*KeyMap).setMaxKey(110);
      (*KeyMap).printPartitions();
      cout << endl;
  
      cout << "[36, 64]" << endl;
      vector<int> v = (*KeyMap).getPartitions(36,true,64,true);
      for(int i=0; i < v.size(); i++)
      cout << v[i] << " " << endl;
      cout << endl;
  
      cout << "(36, 64]" << endl;
      v = (*KeyMap).getPartitions(36,false,64,true);
      for(int i=0; i < v.size(); i++)
      cout << v[i] << " " << endl;
      cout << endl;
  
      cout << "[36, 64)" << endl;
      v = (*KeyMap).getPartitions(36,true,64,false);
      for(int i=0; i < v.size(); i++)
      cout << v[i] << " " << endl;
      cout << endl;
  
      cout << "(36, 64)" << endl;
      v = (*KeyMap).getPartitions(36,false,64,false);
      for(int i=0; i < v.size(); i++)
      cout << v[i] << " " << endl;
      cout << endl;
  
      cout << "get range of partition 3" << endl;
      pair<int,int> p = (*KeyMap).getBoundaries(3);
      cout << "[" << p.first << ", " << p.second << ")" << endl;
      cout << endl;
    */
    return 0;
}
