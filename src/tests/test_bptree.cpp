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
#include "workload/workload.h"

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

// How many keys to produce
const int MAX_KEYS = 100000;

// How many threads will be created
const int NUM_THREADS = 20;

// Tree constants

#ifdef __SUNPRO_CC
// (ip) In Lomond the sizes are different need configuration!
//

// Config-1: 128B NODE SIZE - 4 NODEENTRIES - 5 LEAFENTRIES
//const unsigned cNodeSize = 128;
//const unsigned cLeafSize = 128;
//const unsigned cINodeEntries = 4;
//const unsigned cLeafEntries = 5;
//const unsigned cINodePad = 4;
//const unsigned cLeafPad = 4;

// Config-2: 1KB NODE SIZE - 79 NODEENTRIES - 120 LEAFENTRIES
const unsigned cNodeSize = 1024;
const unsigned cLeafSize = 1024;
const unsigned cINodeEntries = 79;
const unsigned cLeafEntries = 120;
const unsigned cINodePad = 4;
const unsigned cLeafPad = 4;

// Config-3: 4KB NODE SIZE -255 NODEENTRIES - 510 LEAFENTRIES
//const unsigned cNodeSize = 4096;
//const unsigned cLeafSize = 4096;
//const unsigned cINodeEntries = 510;
//const unsigned cLeafEntries = 255;
//const unsigned cINodePad = 4;
//const unsigned cLeafPad = 4;

#else

// Config-1: 128B NODE SIZE - 4 NODEENTRIES - 5 LEAFENTRIES
//const unsigned cNodeSize = 128;
//const unsigned cLeafSize = 128;
//const unsigned cINodeEntries = 4;
//const unsigned cLeafEntries = 5;
//const unsigned cINodePad = 4;
//const unsigned cLeafPad = 4;

// Config-2: 1KB NODE SIZE - 79 NODEENTRIES - 120 LEAFENTRIES
const unsigned cNodeSize = 1024;
const unsigned cLeafSize = 1024;
const unsigned cINodeEntries = 79;
const unsigned cLeafEntries = 120;
const unsigned cINodePad = 4;
const unsigned cLeafPad = 4;

// Config-3: 4KB NODE SIZE -255 NODEENTRIES - 510 LEAFENTRIES
//const unsigned cNodeSize = 4096;
//const unsigned cLeafSize = 4096;
//const unsigned cINodeEntries = 510;
//const unsigned cLeafEntries = 255;
//const unsigned cINodePad = 4;
//const unsigned cLeafPad = 4;

#endif


const unsigned cArch = 64;

// The B+ tree
BPlusTree<int, int, cINodeEntries, cLeafEntries, cINodePad, cLeafPad, cArch> aTree;


// Function declaration
void checkValues(int threads, int keys);


class bptree_loader_t : public thread_t 
{
private:

    int _tid;
    int _max_keys;

    void loadValues() {

        boost::rand48 rng(_tid);

        // Load MAX_KEYS keys
        int avalue = 0;

        for (int i = (_max_keys* _tid); i<(_max_keys*(_tid+1)); i++) {
            avalue = rng();

            // Flips a coin and with probability 0.2 
            // instead of insert makes a probe

            if ( (avalue % 10) < 3 ) {
                TRACE ( TRACE_DEBUG,  "FIND K=(%d)\n", i - _max_keys);
                if (aTree.find(i - MAX_KEYS, &avalue)) {
                    TRACE( TRACE_DEBUG, "Found (%d) = %d\n", i - _max_keys, avalue);
                }
                else {
                    TRACE( TRACE_DEBUG, "Not Found (%d)\n", i - _max_keys);
                }
            }
            else {
                TRACE ( TRACE_DEBUG,  "INSERT K=(%d) V=(%d)\n", i, avalue);
                aTree.insert(i, avalue);
            }
        }
    }    
    
public:

    bptree_loader_t(const c_str& name, const int id, const int keys)
        : thread_t(name),
          _tid(id)
    {

        if (keys > 0) {
            _max_keys = keys;
        }
        else {
            _max_keys = MAX_KEYS;
        }

        TRACE( TRACE_DEBUG,
               "B+ tree loader with id (%d) will load (%d) keys...\n",
               _tid, _max_keys);
    }

    virtual ~bptree_loader_t() { }

    virtual void work() {
        loadValues();
    }

}; // EOF: bptree_loader_t


int main(int argc, char* argv[]) 
{
    int numOfKeys = MAX_KEYS;
    int numOfThreads = NUM_THREADS;

    //    std::cout << aTree << "\n";

    // Parse user input
    if (argc > 2) {
        numOfKeys = atoi(argv[2]);
    }

    if (argc > 1) {
        numOfThreads = atoi(argv[1]);
    }
   
    thread_init();

    //    TRACE_SET( TRACE_ALWAYS | TRACE_DEBUG | TRACE_LOCK_ACTION );
    TRACE_SET( TRACE_ALWAYS );

    TRACE ( TRACE_ALWAYS, "%s <threads> <keys>\n", argv[0]);

    // Validate tree parameters
    BOOST_STATIC_ASSERT((cNodeSize - 8 * cINodeEntries - 8) > 0);
    BOOST_STATIC_ASSERT((cLeafSize - 16 * cLeafEntries - 8) > 0);

    assert(cINodePad < cNodeSize);
    assert(cLeafPad < cLeafSize);
    assert(cINodePad > 0);
    assert(cLeafPad > 0);

    // Validate user input
    assert (numOfThreads > 0);
    assert (numOfKeys > 0);

    TRACE( TRACE_ALWAYS, "Threads = (%d). Keys = (%d)\n", numOfThreads, numOfKeys);

    array_guard_t<pthread_t> bpt_loader_ids = new pthread_t[numOfThreads];

    // Start measuring experiment duration
    time_t tstart = time(NULL);

    try {

        // Create loader threads
        for (int i=0; i<numOfThreads; i++) {

            // Give thread a name
            c_str thr_name("BPL-%d", i+1);

            // Create thread context
            bptree_loader_t* bploader = new bptree_loader_t(thr_name, i, numOfKeys);

            // Create and start running thread
            bpt_loader_ids[i] = thread_create(bploader);
        }

    }
    catch (QPipeException &e) {

        TRACE( TRACE_ALWAYS, "Exception thrown in MT B+ tree loading....\n");
        throw e;
    }

    workload::workload_t::wait_for_clients(bpt_loader_ids, numOfThreads);

    time_t tstop = time(NULL);

    TRACE( TRACE_ALWAYS, "*******************************\n");
    TRACE( TRACE_ALWAYS, "Loading finished\n");
    TRACE( TRACE_ALWAYS, "Threads       = %d\n", numOfThreads);
    TRACE( TRACE_ALWAYS, "KeysPerThread = %d\n", numOfKeys);
    TRACE( TRACE_ALWAYS, "Total Keys    = %d\n", (numOfThreads * numOfKeys));
    TRACE( TRACE_ALWAYS, "Duration      = %d secs\n", tstop - tstart); 
    TRACE( TRACE_ALWAYS, "Rate          = %.2f keys/sec\n", 
           (double)(numOfThreads * numOfKeys)/(double)(tstop - tstart));
    TRACE( TRACE_ALWAYS, "*******************************\n");

    // Print final tree data
    //    aTree.print();

    // Call the tree.find() function
    //checkValues();

    return (0);
}
 


void checkValues(int threads, int keys)
{    
    // Retrieve the loaded keys
    int anotherValue = 0;
    for (int i = 0; i < (threads*keys); i++) {
        if (aTree.find(i, &anotherValue)) {
            TRACE( TRACE_DEBUG, "Found (%d) = %d\n", i, anotherValue);
        }
        else {
            TRACE( TRACE_DEBUG, "Not Found (%d)\n", i);
        }
    }
}

