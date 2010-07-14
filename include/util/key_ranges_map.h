#ifndef __KEY_RANGES_MAP_H
#define __KEY_RANGES_MAP_H

#include <iostream>
#include <map>
#include <vector>
#include <utility>

using namespace std;

class key_ranges_map
{

private:

      // tid -> range_init_key // TODO: think about how to handle this
      map<int,vector<int> > _partitionMap;
      // offset -> range_init_key
      // map<int,int> _offsetMap;
      // range_init_key -> range_end_key
      map<int,int> _keyRangesMap;
      int _numPartitions;
      int _minKey;
      int _maxKey;

      typedef map<int,int>::iterator keysIter;
      typedef map<int,vector<int> >::iterator partitionMapIter;

public:

  // TODO: the first argument ipo talked about
  key_ranges_map(int minKey, int maxKey, int numPartitions);
  // makes equal length partitions from scratch
  void makeEqualPartitions();
  // splits the partition "key" is in into two starting from the "key"
  void addPartition(int key);
  // puts the partition "key" is in into the previous partition
  void deletePartitionWithKey(int key);
  // puts the given partition into previous partition
  void deletePartition(int partition);
  // gets the partition number of the given key
  // TODO: not sure about what should this return though
  int getPartititionWithKey(int key);
  // gets the key range of the given partition
  // pair<int,int> getPartitition(int partition);
  // setters, TODO: decide what to do after you set these values, what seems reasonable to me
  // is change the partition structre as less as possible because later with dynamic load
  // balancing things should adjust properly
  void setNumPartitions(int numPartitions);
  void setMinKey(int minKey);
  void setMaxKey(int maxKey);
  // for debugging
  void printPartitions();

}; // EOF: key_ranges_map

#endif
