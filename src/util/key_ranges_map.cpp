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
 *  @brief:  Implementation of a map of key ranges to partitions. 
 *
 *  @author: Pinar Tozun, July 2010
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

void key_ranges_map::makeEqualPartitions()
{
    // TODO: discuss/think about what to do here
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

void key_ranges_map::addPartition(const Key& key)
{
    char* keyS = (char*) malloc(key.size());
    key.copy_to(keyS);
    _rwlock.acquire_write();
    keysIter iter = _keyRangesMap.lower_bound(keyS);
    if (iter != _keyRangesMap.end()) {
        _keyRangesMap[keyS] = iter->second;
        _keyRangesMap[iter->first] = keyS;
        _numPartitions++;
    } // TODO: else give an error message
    _rwlock.release_write();
}

void key_ranges_map::deletePartitionWithKey(const Key& key)
{
    char* keyS = (char*) malloc(key.size());
    key.copy_to(keyS);
    _deletePartitionWithKey(keyS);
    free (keyS);
}

void key_ranges_map::deletePartition(uint partition)
{
    keysIter iter;
    uint i;
    _rwlock.acquire_read();
    for (i = 0, iter = _keyRangesMap.begin(); iter != _keyRangesMap.end() && i < partition; ++iter, i++)
        ;
    _rwlock.release_read();
    if (iter != _keyRangesMap.end())
        _deletePartitionWithKey(iter->first);
}

inline void key_ranges_map::_deletePartitionWithKey(char* key)
{
    _rwlock.acquire_write();
    keysIter iter = _keyRangesMap.lower_bound(key);
    ++iter;
    if(iter == _keyRangesMap.end())
        --iter; // handles if the given key is in the last partition
    _keyRangesMap[iter->first] = _keyRangesMap[iter->second];
    --iter;
    _keyRangesMap.erase(iter);
    _numPartitions--;
    _rwlock.release_write();
}

uint key_ranges_map::operator()(const Key& key)
{
    return getPartitionWithKey(key);
}

uint key_ranges_map::getPartitionWithKey(const Key& key)
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
            return partitionNum;
        }
        partitionNum++;
    }
    // TODO: indicate error because key is not in the map
}

vector<uint> key_ranges_map::getPartitions(const Key& key1, bool key1Included, const Key& key2, bool key2Included) {
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
    for(uint i = partition2; i<=partition1; i++)
        partitions.push_back(i);
    return partitions;
}

void key_ranges_map::getBoundaries(uint partition, pair<cvec_t, cvec_t>& keyRange) {
    keysIter iter;
    uint i;
    _rwlock.acquire_read();
    for (i = 0, iter = _keyRangesMap.begin(); iter != _keyRangesMap.end() && i< partition; ++iter, i++)
        ;
    _rwlock.release_read();
    // TODO: Not sure whether this is correct, should check
    keyRange.first.put(iter->first, sizeof(iter->first));
    keyRange.second.put(iter->second, sizeof(iter->second));
}

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
