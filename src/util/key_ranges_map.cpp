
#include "util/key_ranges_map.h"

//#include "../../include/util/key_ranges_map.h"

/** key_ranges_map interface */

key_ranges_map::key_ranges_map(int minKey, int maxKey, int numPartitions)
	: _minKey(minKey), _maxKey(maxKey), _numPartitions(numPartitions) {
	makeEqualPartitions();
}

void key_ranges_map::makeEqualPartitions()
{
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
}

void key_ranges_map::addPartition(int key)
{
	_rwlock.acquire_write();
	keysIter iter = _keyRangesMap.lower_bound(key);
	if (iter != _keyRangesMap.end()) {
	  _keyRangesMap[key] = iter->second;
	  _keyRangesMap[iter->first] = key;
	  _numPartitions++;
	 } // TODO: else give an error message
	_rwlock.release_write();
}

void key_ranges_map::deletePartitionWithKey(int key)
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

void key_ranges_map::deletePartition(int partition)
{
	keysIter iter;
	int i;
	_rwlock.acquire_read();
	for (i = 0, iter = _keyRangesMap.begin(); iter != _keyRangesMap.end() && i
			< partition; ++iter, i++)
		;
	_rwlock.release_read();
	if (iter != _keyRangesMap.end())
		deletePartitionWithKey(iter->first);
}

int key_ranges_map::operator()(int key)
{
	return getPartitionWithKey(key);
}

int key_ranges_map::getPartitionWithKey(int key)
{
	// TODO: if this just wants the init_key as the return value
	// you can easily change it to
	// keysIter iter = _keyRangesMap.lower_bound(key);
	// return iter->first;
	keysIter iter;
	int partitionNum = 0;
	_rwlock.acquire_read();
	for (iter = _keyRangesMap.begin(); iter != _keyRangesMap.end(); ++iter) {
		if (key >= iter->first && key < iter->second) {
			_rwlock.release_read();
			return partitionNum;
		}
		partitionNum++;
	}
	// TODO: indicate error because key is not in the map
}

vector<int> key_ranges_map::getPartitions(int key1, bool key1Included, int key2, bool key2Included) {
	vector<int> partitions;
	// find the partition key1 is in
	int partition1 = getPartitionWithKey(key1);
	// find the partition key2 is in
	int partition2 = getPartitionWithKey(key2);
	// check corner cases
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
	for(int i = partition2; i<=partition1; i++)
	  partitions.push_back(i);
	return partitions;
}

pair<int, int> key_ranges_map::getBoundaries(int partition) {
  pair<int,int> keyRange;
  keysIter iter;
  int i;
  _rwlock.acquire_read();
  for (i = 0, iter = _keyRangesMap.begin(); iter != _keyRangesMap.end() && i< partition; ++iter, i++)
    ;
  _rwlock.release_read();
  keyRange.first = iter->first;
  keyRange.second = iter->second;
  return keyRange;
}

void key_ranges_map::printPartitions()
{
	keysIter iter;
	int i = 0;
	_rwlock.acquire_read();
	for (iter = _keyRangesMap.begin(); iter != _keyRangesMap.end(); ++iter, i++)
		cout << "Partition: " << i << "\tStartKey: " << iter->first
				<< "\tEndKey: " << iter->second << endl;
	_rwlock.release_read();
}

void key_ranges_map::setNumPartitions(int numPartitions)
{
	_rwlock.acquire_write();
	_numPartitions = numPartitions;
	// TODO: update partitions
	_rwlock.release_write();
}

void key_ranges_map::setMinKey(int minKey)
{
	_rwlock.acquire_write();
	_minKey = minKey;
	keysIter iter = _keyRangesMap.end();
	iter--;
	_keyRangesMap[_minKey] = iter->second;
	_keyRangesMap.erase(iter);
	_rwlock.release_write();
}

void key_ranges_map::setMaxKey(int maxKey)
{
	_rwlock.acquire_write();
	_maxKey = maxKey;
	keysIter iter = _keyRangesMap.begin();
	iter->second = _maxKey + 1;
	_rwlock.release_write();
}

int main(void)
{
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

	return 0;
}
