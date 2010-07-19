#ifndef __KEY_RANGES_MAP_H
#define __KEY_RANGES_MAP_H

#include <iostream>
#include <map>
#include <vector>
#include <utility>

#include "util.h"

using namespace std;

class key_ranges_map
{
private:

	// range_init_key -> range_end_key
	map<int, int, std::greater<int> > _keyRangesMap;
	int _numPartitions;
	int _minKey;
	int _maxKey;
	// for thread safety multiple readers/single writer lock
	occ_rwlock _rwlock;

	typedef map<int, int>::iterator keysIter;

public:

	// equally partitions the given key range ([minKey,maxKey]) depending on the given partition number
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
	int getPartitionWithKey(int key);
	int operator()(int key);
	// returns the list of partitionIDs that covers [key1, key2], (key1, key2], [key1, key2), or (key1, key2) ranges
	vector<int> getPartitions(int key1, bool key1Included, int key2,
			bool key2Included);
	// returns the range boundaries of the partition
	pair<int, int> getBoundaries(int partition);
	// setters
	// TODO: decide what to do after you set these values, what seems reasonable to me
	// is change the partition structure as less as possible because later with dynamic load
	// balancing things should adjust properly
	void setNumPartitions(int numPartitions);
	void setMinKey(int minKey);
	void setMaxKey(int maxKey);
	// for debugging
	void printPartitions();

}; // EOF: key_ranges_map

#endif
