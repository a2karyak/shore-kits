
#include "util/key_ranges_map.h"

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
			return partitionNum;
		}
		partitionNum++;
	}
	_rwlock.release_read();
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
	key_ranges_map* KeyMap = new key_ranges_map(10, 100, 10);
	(*KeyMap).printPartitions();
	cout << endl;

	(*KeyMap).addPartition(60);
	(*KeyMap).printPartitions();
	cout << endl;

	(*KeyMap).deletePartitionWithKey(20);
	(*KeyMap).printPartitions();
	cout << endl;

	(*KeyMap).deletePartition(9);
	(*KeyMap).printPartitions();
	cout << endl;

	cout << (*KeyMap).getPartitionWithKey(70) << endl;
	cout << endl;

	cout << (*KeyMap)(70) << endl;
	cout << endl;

	(*KeyMap).setMinKey(6);
	(*KeyMap).printPartitions();
	cout << endl;

	(*KeyMap).setMaxKey(110);
	(*KeyMap).printPartitions();
	cout << endl;

	return 0;
}
