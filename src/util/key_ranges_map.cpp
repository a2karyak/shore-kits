#include <iostream>

#include "../../include/util/key_ranges_map.h"

/** key_ranges_map interface */

key_ranges_map::key_ranges_map(int minKey, int maxKey, int numPartitions)
  : _minKey(minKey), _maxKey(maxKey), _numPartitions(numPartitions)
{
  makeEqualPartitions();
}

void key_ranges_map::makeEqualPartitions()
{
  _partitionMap.clear();
  _keyRangesMap.clear();
  int range = (_maxKey - _minKey) / _numPartitions;
  int currentLower = 0;
  int currentUpper = _minKey;
  for(int i=0; i<_numPartitions; i++) {
    currentLower = currentUpper;
    currentUpper = currentLower + range;
    _keyRangesMap[currentLower] = currentUpper;
    // _offsetMap[i] = currentLower;
  }
  _keyRangesMap[currentLower] = _maxKey + 1;
}

void key_ranges_map::addPartition(int key)
{
  int partitionNum = 0;
  keysIter iter;
  for(iter = _keyRangesMap.begin(); iter != _keyRangesMap.end(); ++iter) {
    if(key > iter->first && key < iter->second) {
      	_keyRangesMap[key] = iter->second;
	_keyRangesMap[iter->first] = key;
	//for(int i = _numPartitions; i > partitionNum+1; i--)
	//   _offsetMap[i] = _offsetMap[i-1];
	//_offsetMap[partitionNum+1] = key;
	_numPartitions++;
	return;
    }
    partitionNum++;
  }
}

void key_ranges_map::deletePartitionWithKey(int key)
{
  keysIter iter;
  int prevKey = 0;
  for(iter = _keyRangesMap.begin(); iter != _keyRangesMap.end(); ++iter) {
    if(key >= iter->first && key < iter->second) {
      if(iter == _keyRangesMap.begin()) {
	prevKey = iter->first;
	++iter; // TODO: if this is null give error
      }
      _keyRangesMap[prevKey] = iter-> second;
      _keyRangesMap.erase(iter);
      _numPartitions--;
      return;
    }
    prevKey = iter->first;
  }
}

void key_ranges_map::deletePartition(int partition)
{
  keysIter iter;
  int i;
  for(i=0, iter = _keyRangesMap.begin(); iter != _keyRangesMap.end() && i < partition; ++iter, i++)
    ;
  if(iter != _keyRangesMap.end())
    deletePartitionWithKey(iter->first);
}

int key_ranges_map::getPartititionWithKey(int key)
{
  keysIter iter;
  int partitionNum = 0;
  for(iter = _keyRangesMap.begin(); iter != _keyRangesMap.end(); ++iter) {
    if(key >= iter->first && key < iter->second) {
    	return partitionNum;
    }
    partitionNum++;
  }
}

//pair<int,int> key_ranges_map::getPartitition(int partition) {
//  return pair<int, int>(_offsetMap[partition], _keyRangesMap[_offsetMap[partition]]);
//}

void key_ranges_map::printPartitions()
{
  keysIter iter;
  int i = 0;
  for(iter = _keyRangesMap.begin(); iter != _keyRangesMap.end(); ++iter, i++)
    cout << "Partition: " << i << "\tStartKey: " << iter->first << "\tEndKey: " << iter->second << endl;
}

void key_ranges_map::setNumPartitions(int numPartitions)
{
  _numPartitions = numPartitions;
  // TODO: update partitions
}

void key_ranges_map::setMinKey(int minKey)
{
  _minKey = minKey;
  keysIter iter = _keyRangesMap.begin();
  _keyRangesMap[_minKey] = iter->second;
  _keyRangesMap.erase(iter);
}

void key_ranges_map::setMaxKey(int maxKey)
{
  _maxKey = maxKey;
  keysIter iter = _keyRangesMap.end();
  iter--;
  iter->second = _maxKey+1;
}

int main(void)
{
  key_ranges_map* KeyMap = new key_ranges_map(10, 100, 10);
  (*KeyMap).printPartitions();
  cout << endl;

  (*KeyMap).addPartition(60);
  (*KeyMap).printPartitions();
  cout << endl;

  (*KeyMap).deletePartitionWithKey(20);
  (*KeyMap).printPartitions();
  cout << endl;

  (*KeyMap).deletePartition(0);
  (*KeyMap).printPartitions();
  cout << endl;

  cout << (*KeyMap).getPartititionWithKey(70) << endl;
  cout << endl;

  (*KeyMap).setMinKey(6);
  (*KeyMap).printPartitions();
  cout << endl;

  (*KeyMap).setMaxKey(110);
  (*KeyMap).printPartitions();
  cout << endl;

  return 0;
}
