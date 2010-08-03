#ifndef __KEY_RANGES_MAP_H
#define __KEY_RANGES_MAP_H

#include <iostream>
#include <cstring> 
#include <map>
#include <vector>
#include <utility>

#include "util.h"

#include "sm_vas.h"

using namespace std;

// For map<char*,char*> to compare char*
struct cmp_str_greater
{
  bool operator()(char const *a, char const *b)
  {
    return strcmp(a,b) > 0;
  }
};

class key_ranges_map
{
 private:

  typedef map<char*, char*>::iterator keysIter;
  typedef cvec_t Key;

  // range_init_key -> range_end_key
  map<char*, char*, cmp_str_greater > _keyRangesMap;
  uint _numPartitions;
  char* _minKey;
  char* _maxKey;
  // for thread safety multiple readers/single writer lock
  occ_rwlock _rwlock;

  // puts the partition "key" is in into the previous partition
  void _deletePartitionWithKey(char* key);
  // gets the partition number of the given key
  uint _getPartitionWithKey(char* key);
  // gets the string that describes the content of the key
  char* _getKey(const Key& key);

 public:
  
  // TODO: equally partitions the given key range ([minKey,maxKey]) depending on the given partition number
  key_ranges_map(const Key& minKey, const Key& maxKey, uint numPartitions);	
  // makes equal length partitions from scratch
  void makeEqualPartitions();
  // splits the partition "key" is in into two starting from the "key"
  void addPartition(const Key& key);
  // puts the partition "key" is in into the previous partition
  void deletePartitionWithKey(const Key& key);
  // puts the given partition into previous partition
  void deletePartition(uint partition);
  // gets the partition number of the given key
  uint getPartitionWithKey(const Key& key);
  uint operator()(const Key& key);
  // returns the list of partitionIDs that covers [key1, key2], (key1, key2], [key1, key2), or (key1, key2) ranges
  vector<uint> getPartitions(const Key& key1, bool key1Included, const Key& key2, bool key2Included);
  // returns the range boundaries of the partition
  void getBoundaries(uint partition, pair<cvec_t, cvec_t>& keyRange);
  // setters
  // TODO: decide what to do after you set these values, what seems reasonable to me
  // is change the partition structure as less as possible because later with dynamic load
  // balancing things should adjust properly
  void setNumPartitions(uint numPartitions);
  void setMinKey(const Key& minKey);
  void setMaxKey(const Key& maxKey);
  // for debugging
  void printPartitions(); 
    
}; // EOF: key_ranges_map

#endif
