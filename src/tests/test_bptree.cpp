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

#include <sstream>

using std::cout;
using std::endl;


// How many keys to produce
//const int MAX_KEYS = 10000;
const int MAX_KEYS = 300;
//const int MAX_KEYS = 30;

// How many threads will be created
//const int NUM_THREADS = 1;
//const int NUM_THREADS = 2;
const int NUM_THREADS = 20;



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
void checkValues();


class bptree_loader_t : public thread_t 
{
private:

    int _tid;

    void loadValues() {

        boost::rand48 rng(_tid);

        // Load MAX_KEYS keys
        int avalue = 0;

        for (int i = MAX_KEYS* _tid; i<MAX_KEYS*(_tid+1); i++) {
            avalue = rng();

            // Modification, flips a coin and with probability 0.2 
            // instead of insert makes a probe

            if ( (avalue % 10) < 3 ) {
                TRACE ( TRACE_DEBUG,  "FIND K=(%d)\n", i - MAX_KEYS);
                if (aTree.find(i - MAX_KEYS, &avalue)) {
                    TRACE( TRACE_DEBUG, "Found (%d) = %d\n", i - MAX_KEYS, avalue);
                }
                else {
                    TRACE( TRACE_DEBUG, "Not Found (%d)\n", i - MAX_KEYS);
                }
            }
            else {
                TRACE ( TRACE_DEBUG,  "INSERT K=(%d) V=(%d)\n", i, avalue);
                aTree.insert(i, avalue);
            }
        }
    }    
    
public:

    bptree_loader_t(const c_str& name, const int id)
        : thread_t(name),
          _tid(id)
    {
        TRACE( TRACE_DEBUG,
               "B+ tree loader with id (%d) constructed...\n",
               _tid);
    }

    virtual ~bptree_loader_t() { }

    virtual void* run() {

        loadValues();
        return (NULL);
    }

}; // EOF: bptree_loader_t


int main(void)
{
    thread_init();

    TRACE_SET( TRACE_ALWAYS | TRACE_DEBUG );
    //TRACE_SET( TRACE_ALWAYS );
    
    BOOST_STATIC_ASSERT(cTwoLines - 16 * cINodeEntries - 8);
    BOOST_STATIC_ASSERT(cTwoLines - 16 * cLeafEntries - 8);    

    assert (cINodePad > 0);
    assert (cLeafPad > 0);

    array_guard_t<pthread_t> bpt_loader_ids = new pthread_t[NUM_THREADS];

    time_t tstart = time(NULL);

    int i = 0;

    try {

        // Create loader threads
        for (i=0; i<NUM_THREADS; i++) {

            // Give thread a name
            c_str thr_name("BPL-%d", i+1);

            // Create thread context
            bptree_loader_t* bploader = new bptree_loader_t(thr_name, i);

            // Create and start running thread
            bpt_loader_ids[i] = thread_create(bploader);
                
            //pthread_create(&tid[i], NULL, loadValues, reinterpret_cast<void*>(i));
        }

    }
    catch (QPipeException &e) {

        TRACE( TRACE_ALWAYS, "Exception thrown in MT B+ tree loading....\n");
        throw e;
    }

    workload::workload_t::wait_for_clients(bpt_loader_ids, NUM_THREADS);

    time_t tstop = time(NULL);

    TRACE( TRACE_ALWAYS, "*******************************\n");
    TRACE( TRACE_ALWAYS, "Loading finished\n");
    TRACE( TRACE_ALWAYS, "Keys     = %d\n", MAX_KEYS * NUM_THREADS);
    TRACE( TRACE_ALWAYS, "Duration = %d secs\n", tstop - tstart); 
    TRACE( TRACE_ALWAYS, "Rate     = %.2f keys/sec\n", 
           (double)(MAX_KEYS * NUM_THREADS)/(double)(tstop - tstart));
    TRACE( TRACE_ALWAYS, "*******************************\n");

    // Print final tree data
    //    aTree.print();

    // Call the tree.find() function
    //checkValues();

    return (0);
}
 


void checkValues()
{    
    // Retrieve the loaded keys
    int anotherValue = 0;
    for (int i = 0; i < NUM_THREADS * MAX_KEYS; i++) {
        if (aTree.find(i, &anotherValue)) {
            TRACE( TRACE_DEBUG, "Found (%d) = %d\n", i, anotherValue);
        }
        else {
            TRACE( TRACE_DEBUG, "Not Found (%d)\n", i);
        }
    }
}

