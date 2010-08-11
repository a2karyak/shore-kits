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


/****************************************************************** 
 *
 * Construction/Destruction
 *
 * @brief: The constuctor calls the default initialization function
 * @brief: The destructor needs to free all the malloc'ed keys
 *
 ******************************************************************/

key_ranges_map::key_ranges_map(const Key& minKey, const Key& maxKey, const uint numParts)
{
    // TODO: call a function that makes sure that no such store exists 

    // TODO: create a store
    
    // Calls the default initialization. 
    uint p = makeEqualPartitions(minKey,maxKey,numParts);
    fprintf (stdout, "%d partitions created", p);
}


key_ranges_map::~key_ranges_map()
{
    // Delete the allocated keys in the map
    keysIter iter;
    uint i=0;
    _rwlock.acquire_write();
    for (iter = _keyRangesMap.begin(); iter != _keyRangesMap.end(); ++iter, ++i) {
        TRACE( TRACE_DEBUG, "Partition %d\tStart (%s)\n", i, iter->first);
        free (iter->first);
    }

    // Delete the boundary keys
    free (_minKey);
    free (_maxKey);
    _rwlock.release_write();    
}


/****************************************************************** 
 *
 * @fn:     makeEqualPartitions()
 *
 * @brief:  Makes equal length partitions from scratch
 *
 * @return: The number of partitions that were actually created
 *
 ******************************************************************/

uint key_ranges_map::makeEqualPartitions(const Key& minKey, const Key& maxKey, 
                                         const uint numParts)
{
    assert (minKey<=maxKey);

    // TODO: make sure that the store does not have any entries (isClean())

    // TODO: call _distributeSpace!

    _rwlock.acquire_write();

    // Set min/max keys
    uint minsz = minKey.size();
    uint maxsz = maxKey.size();    
    _minKey = (char*) malloc(minsz);
    _maxKey = (char*) malloc(maxsz);    
    minKey.copy_to(_minKey);
    maxKey.copy_to(_maxKey);

    uint keysz = max(minsz,maxsz); // Use that many chars for the keys entries

    uint partsCreated = 0; // In case it cannot divide to numParts partitions

    // Tread the cvec_t's as integers
    double dmin = (double)(*(int*)_minKey);
    double dmax = (double)(*(int*)_maxKey);

    // TODO: This may assert if min/max keys not of equal lenght. If minKey longer
    //       then the corresponding int value will be larger than the maxKey. It
    //       should do some normalization (for example, append zeroes to the max).
    assert (!(dmin>dmax)); 

    double range = (dmax-dmin)/(double)numParts;
    lpid_t subtreeroot;

    _keyRangesMap.clear();
    for (double lowerKey=dmin; lowerKey<=dmax; lowerKey+=range, ++partsCreated) {
	char* key = (char*) malloc(keysz+1);
        memset(key,0,keysz+1); 
	snprintf(key, keysz, "%f", lowerKey);

        // TODO: call shore to create sub-tree and use the return lpid_t
	subtreeroot = lpid_t();
       
_keyRangesMap[key] = subtreeroot;
    }
    _rwlock.release_write();
    assert (partsCreated != 0); // Should have created at least one partition
    return (partsCreated);
}



/****************************************************************** 
 *
 * @fn:      _distributeSpace()
 *
 * @brief:   Helper function, which tries to evenly distribute the space between
 *           two strings to a certain number of subspaces
 *
 * @param:   const char* A    - The beginning of the space
 * @param:   const char* B    - The end of the space
 * @param:   const uint sz    - The common size of the two strings
 * @param:   const uint parts - The number of partitions which should be created
 * @param:   char** subparts  - An array of strings with the boundaries of the
 *                              distribution
 * 
 * @returns: The number of partitions actually created
 *
 ******************************************************************/

uint key_ranges_map::_distributeSpace(const char* A, 
                                      const char* B, 
                                      const uint sz, 
                                      const uint parts, 
                                      char** subparts)
{
    return (0); // IP: TODO!
    
    // IP: The code below it won't compile but it should be close to.
    //     This will be called only at the initialization so we don't
    //     care about performance!

    // uint pcreated = 0;

    // assert (strncmp(A,B,sz)); // It should A<B
    // subparts = (char**)malloc(parts*sizeof(char*));

    // uint base=0;
    // while ((A[base] == B[base]) && (base<sz)) {
    //     // discard the common prefixes
    //     base++; 
    // }

    // // find the [base+idx] space which can be divided evenly
    // uint idx=0;
    // uint ia=0;
    // uint ib=0;
    // uint range=0;
    // do {
    //     // Get an integer value for the substrings and check if they
    //     // can be divided evenly
    //     ia = atoi(substring(A,base, base+idx));
    //     ib = atoi(substring(B,base,base+idx));
    //     range = (ib-ia)/subparts;
    // } while ((range==0) && ((base+idx)<sz));
        
    // // create the strings
    // char* lower = strdup(A);
    // while (strncmp(lower,B,sz) && (pcreated<parts)) {
    //     subparts[pcreated++] = strdup(lower);
    //     lower[base..base+idx] = lower[base..base+idx] + range;		
    // }
    // return (pcreated);
}



/****************************************************************** 
 *
 * @fn:    addPartition()
 *
 * @brief: Splits the partition where "key" belongs to two partitions. 
 *         The start of the second partition is the "key".
 *
 ******************************************************************/

w_rc_t key_ranges_map::_addPartition(char* keyS, lpid_t& root)
{
    w_rc_t r = RCOK;

    _rwlock.acquire_write();
    keysIter iter = _keyRangesMap.lower_bound(keyS);
    if (iter != _keyRangesMap.end()) {
	root = lpid_t();
	// TODO: call a function from shore to initialize lpid_t
        _keyRangesMap[keyS] = root;
        _numPartitions++;
    }
    else {
        r = RC(mrb_PARTITION_NOT_FOUND);
    }
    _rwlock.release_write();

    return (r);
}

w_rc_t key_ranges_map::addPartition(const Key& key, lpid_t& root)
{
    char* keyS = (char*) malloc(key.size());
    key.copy_to(keyS);
    return (_addPartition(keyS,root));
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

w_rc_t key_ranges_map::_deletePartitionByKey(char* keyS)
{
    w_rc_t r = RCOK;

    _rwlock.acquire_write();
    keysIter iter = _keyRangesMap.lower_bound(keyS);

    if(iter == _keyRangesMap.end()) {
	// partition not found, return an error
	return (RC(mrb_PARTITION_NOT_FOUND));
    }
    lpid_t root1 = iter->second;
    ++iter;
    if(iter == _keyRangesMap.end()) {
	--iter;
	if(iter == _keyRangesMap.begin()) {
	    // partition is the last partition, cannot be deleted
	    return (RC(mrb_LAST_PARTITION));
	}
	lpid_t root2 = root1;
	--iter;
	root1 = iter->second;
    }
    else {
	lpid_t root2 = iter->second;
	--iter;
    }
    // TODO: call some function from shore that merges root1&root2
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
    r = _deletePartitionByKey(keyS);
    free (keyS);
    return (r);
}

w_rc_t key_ranges_map::deletePartition(lpid_t pid)
{
    w_rc_t r = RCOK;
    bool bFound = false;

    keysIter iter;
    _rwlock.acquire_read();
    for (iter = _keyRangesMap.begin(); iter != _keyRangesMap.end(); ++iter) {
        if (iter->second == pid) {
            bFound = true;
	    break;
        }
    }
    _rwlock.release_read();

    if (bFound) {
        r = _deletePartitionByKey(iter->first);
    } else {
	return (RC(mrb_PARTITION_NOT_FOUND));
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

    _rwlock.acquire_read();
    keysIter iter = _keyRangesMap.lower_bound(keyS);
    free (keyS);
    if(iter == _keyRangesMap.end()) {
	// the key is not in the map, returns error.
	return (RC(mrb_PARTITION_NOT_FOUND));
    }
    pid = iter->second;
    _rwlock.release_read();
    return (RCOK);    
}

w_rc_t key_ranges_map::operator()(const Key& key, lpid_t& pid)
{
    return (getPartitionByKey(key,pid));
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
    
    if(key2 < key1) {
	// return error if the bounds are not given correctly
	return (RC(mrb_KEY_BOUNDARIES_NOT_ORDERED));
    }  
    char* key1S = (char*) malloc(key1.size());
    key1.copy_to(key1S);
    char* key2S = (char*) malloc(key2.size());
    key2.copy_to(key2S);

    _rwlock.acquire_read();
    keysIter iter1 = _keyRangesMap.lower_bound(key1S);
    keysIter iter2 = _keyRangesMap.lower_bound(key2S);
    if (iter1 == _keyRangesMap.end() || iter2 == _keyRangesMap.end()) {
	// at least one of the keys is not in the map, returns error.
	return (RC(mrb_PARTITION_NOT_FOUND));
    }
    // TODO: think about how to check key1 not being included
    // the corner case here is that when key1 is the last key from the its partition
    // if it's not included then we shouldn't take that partition
    // but how to check the last key of a partition ??
    while (iter1 != iter2) {
	pidVec.push_back(iter1->second);
	iter1++;
    }
    // check for !key2Included
    if(key2Included || strcmp(iter1->first,key2S)!=0)
	pidVec.push_back(iter1->second);
    _rwlock.release_read();

    return (r);
}


/****************************************************************** 
 *
 * @fn:    getBoundaries()
 *
 * @brief: Returns the range boundaries of a partition
 *
 ******************************************************************/

w_rc_t key_ranges_map::getBoundaries(lpid_t pid, pair<cvec_t, cvec_t>& keyRange) 
{
    keysIter iter;
    bool bFound = false;

    _rwlock.acquire_read();
    for (iter = _keyRangesMap.begin(); iter != _keyRangesMap.end(); ++iter) {
        if (iter->second == pid) {
            bFound = true;
	    break;
        }
    }
    _rwlock.release_read();
    
    if(!bFound) {
	// the pid is not in the map, returns error.
	return (RC(mrb_PARTITION_NOT_FOUND));
    }

    // TODO: Not sure whether this is correct, should check
    keyRange.first.put(iter->first, sizeof(iter->first));
    iter++;
    if(iter == _keyRangesMap.end()) { 
        // check whether it is the last range
	keyRange.second.put(_maxKey, sizeof(_maxKey));
    }
    else {
        keyRange.second.put(iter->first, sizeof(iter->first));
    }

    return (RCOK);
}


/****************************************************************** 
 *
 * @fn:    getBoundariesVec()
 *
 * @brief: Returns a vector with the key boundaries for all the partitions
 *
 ******************************************************************/

w_rc_t key_ranges_map::getBoundariesVec(vector< pair<char*,char*> >& keyBoundariesVec)
{
    keysIter iter;
    pair<char*, char*> keyPair;
    keyPair.second = NULL;

    _rwlock.acquire_read();
    for (iter = _keyRangesMap.begin(); iter != _keyRangesMap.end(); ++iter) {
        keyPair.first = iter->first;
        if (!keyBoundariesVec.empty()) {
            // Visit the last entry and update the upper boundary
            keyBoundariesVec.back().second = iter->first;
        }
        // Add entry to the vector
        keyBoundariesVec.push_back(keyPair);
    }
    _rwlock.release_read();
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
        TRACE( TRACE_DEBUG, "Partition %d\tStart (%s)\n", i, iter->first);
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
    _rwlock.release_write();
}


uint key_ranges_map::getNumPartitions() const
{
    return (_numPartitions);
}

char* key_ranges_map::getMinKey() const
{
    return (_minKey);
}

char* key_ranges_map::getMaxKey() const
{
    return (_maxKey);
}


#if 0
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
#endif
