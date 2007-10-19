/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file test_bptree.cpp
 *
 *  @brief Test of the In-memory B+ tree implementaion 
 *  (loading, insterting, updating, multi-threaded)
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "util.h"
#include "util/bptree.h"

// For random number generator (rng)
#include <boost/random.hpp>

#ifdef __SUNPRO_CC
#include <time.h>
#include <iostream.h>
#include <string.h>
#else
#include <ctime>
#include <iostream>
#include <string>
#endif

#include <sstream>

using std::cout;
using std::endl;


// How many keys to produce
//const int MAX_KEYS = 10000;
const int MAX_KEYS = 30;

// How many threads will be created
//const int NUM_THREADS = 1;
const int NUM_THREADS = 2;
//const int NUM_THREADS = 20;
pthread_t tid[NUM_THREADS]; // Array of thread IDs

// Tree constants
const unsigned cTwoLines = 128;
const unsigned cINodeEntries = 4;
const unsigned cLeafEntries = 5;
//const unsigned cINodeEntries = 7;
//const unsigned cLeafEntries = 14;
const unsigned cArch = 64;

const unsigned cINodePad = cTwoLines - 16 * cINodeEntries - 8;
const unsigned cLeafPad = cTwoLines - 8 * cLeafEntries - 8;

// The B+ tree
BPlusTree<int, int, cINodeEntries, cLeafEntries, cINodePad, cLeafPad, cArch> aTree;

// Function declaration
void* loadValues(void* parm);
void checkValues(int numOfThreads);

int main(void)
{
    TRACE_SET( TRACE_ALWAYS | TRACE_DEBUG );

    thread_init();

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

    // Print final tree data
    aTree.print();
    //    checkValues(NUM_THREADS);

    return (0);
}
 

void* loadValues(void* parm)
{
    //    int tid = static_cast<int>(parm);
    int tid = (int)((long)parm);

    std::string strout;
    std::ostringstream o;

    cout << "Thread with ID = " << tid << " starts\n";

    boost::rand48 rng(boost::int32_t(2137 + tid));

    // Load MAX_KEYS keys
    int avalue = 0;
    for (int i=MAX_KEYS*tid; i<MAX_KEYS*(tid+1); i++) {
        avalue = rng();

        // Modification, flips a coin and with probability 0.2 
        // instead of insert makes a probe (find)

        /*
        if ( (avalue % 10) < 3 ) {
            if (aTree.find(i, &avalue))
                o << "F (" << i << ") V = " << avalue << "\n";
        }
        else {
        */
            cout << "(" <<  tid <<  ") " << i << " " << avalue << "\n";
            //o << "(" <<  tid <<  ") " << i << " " << avalue << "\n";
            aTree.insert(i, avalue);

            //}
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
