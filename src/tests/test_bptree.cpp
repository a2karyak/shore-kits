/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file test_bptree.cpp
 *
 *  @brief Test of the In-memory B+ tree implementaion 
 *  (loading, insterting, updating, multi-threaded)
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "util/bptree.h"

// For random number generator (rng)
#include <boost/random.hpp>

#include <ctime>
#include <iostream>
#include <sstream>
#include <string>

using std::cout;
using std::endl;


// How many keys to produce
//const int MAX_KEYS = 10000;
const int MAX_KEYS = 100000;

// Tree constants
const unsigned cTwoLines = 128;
const unsigned cINodeEntries = 7;
const unsigned cLeafEntries = 14;
const unsigned cArch = 64;

const unsigned cINodePad = cTwoLines - 16 * cINodeEntries - 8;
const unsigned cLeafPad = cTwoLines - 8 * cLeafEntries - 8;


// How many threads will be created
const int NUM_THREADS = 2;
pthread_t tid[NUM_THREADS]; // Array of thread IDs

// The B+ tree
BPlusTree<int, int, cINodeEntries, cLeafEntries, cINodePad, cLeafPad, cArch> aTree;

// Function declaration
void* loadValues(void* parm);
void checkValues(int numOfThreads);

int main(void)
{
    std::cout << "Hello world!\n"; 

    time_t tstart = time(NULL);
    
    BOOST_STATIC_ASSERT(cTwoLines - 16 * cINodeEntries - 8);
    BOOST_STATIC_ASSERT(cTwoLines - 16 * cLeafEntries - 8);    

    assert (cINodePad > 0);
    assert (cLeafPad > 0);

    cout << aTree;

    int i = 0;

    // Create loader threads
    for (i=0; i<NUM_THREADS; i++) {
        pthread_create(&tid[i], NULL, loadValues, reinterpret_cast<void*>(i));
    }

    // Wait for all the threads to finish
    for (i=0; i<NUM_THREADS; i++) {
        pthread_join(tid[i], NULL);
    }

    time_t tstop = time(NULL);

    cout << "Loading lasted: " << (tstop - tstart) << " secs. " 
         << tstop << " " <<  tstart << "\n";

    //    checkValues(NUM_THREADS);

    return (0);
}
 

void* loadValues(void* parm)
{
    //    int tid = static_cast<int>(parm);
    int tid = (int)parm;

    std::string strout;
    std::ostringstream o;

    cout << "Thread with ID = " << tid << " starts\n";

    boost::rand48 rng(boost::int32_t(2137));

    // Load MAX_KEYS keys
    int avalue = 0;
    for (int i=MAX_KEYS*tid; i<MAX_KEYS*(tid+1); i++) {
        avalue = rng();
        aTree.insert(i, avalue);

        o << "(" <<  tid <<  ") " << i << " " << avalue << "\n";
    }

    //    cout << o.str();
    
    return (NULL);
}

void checkValues(int numOfThreads)
{    
    // Retrieve those MAX_KEYS keys
    int anotherValue = 0;
    for (int i = 0; i < numOfThreads * MAX_KEYS; i++) {
        if (aTree.find(i, &anotherValue))
            cout << "Found (" << i << ") V = " << anotherValue << "\n";
    }
}
