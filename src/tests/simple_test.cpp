/* -*- mode:C++; c-basic-offset:4 -*- */


#include <iostream>
#include "util/stl_pooled_alloc.h"
#include <boost/scoped_ptr.hpp>
#include <ctime>
#include "util.h"
#include <fstream>

#include "util/cache.h"


using namespace std;

ifstream::pos_type size;

static void ShowUsage( Pool* pool )
{
	// print the pool usage to cout
	std::cout << "pool: used: " << pool->GetUsed() 
		<< " overflow: " << pool->GetOverflow() 
		<< std::endl;
}

static void Test1( Pool* pool )
{
    // use the pool with a map of ints to ints
    std::less<int> compare1;
    PooledMap<int, int>::Type test1( compare1, pool );

    test1[0] = 42;
    test1[1] = 48;
    test1[3] = 53;
    test1[0] = 94;
    test1.erase( 3 );

    // use the pool with a list of floats
    PooledList<float>::Type test2( pool );
    test2.push_back( 3.0f );
    test2.push_back( 1.0f );
    test2.pop_front();

    // use the pool with a set of doubles
    std::less<double> compare3;
    PooledSet<double>::Type test3( compare3, pool );
    for( int i = 0; i < 512; ++i )
        test3.insert( double( i ) );

    // print the pool usage
    ShowUsage( pool );
}

template<typename L>
static clock_t StressList( L& list )
{
    static const int maxInsertions = 1600;
    static const int numTests = 100;
    //static const int numTests = 5;

    // fill and clear the list repeatedly
    clock_t start = clock();	
    for( int test = 0; test < numTests; ++test )
	{
            list.clear();
            for( int i = 0; i < maxInsertions; ++i )
                list.push_back( rand() );
	}
    clock_t end = clock();

    // return the total number of ticks for the test
    return end - start;
}

static float Test2( Pool* pool )
{
    PooledList<int>::Type pooledList( pool );
    std::list<int> standardList;

    clock_t pooledTime = StressList( pooledList );
    clock_t standardTime = StressList( standardList );

    float factor = float( pooledTime )/float( standardTime );

    //    std::cout << "Relative: " << factor << std::endl;

//     std::cout << "standard list ticks: " << standardTime << "\n"
//               << "pooled list ticks: " << pooledTime << "\n"
//               << "relative time: " << factor << std::endl;

    return (factor);
}


int first_test()
{
    //
    // test 1
    // 
	
    // allocate a pool of 256 32-byte elements
    boost::scoped_ptr<Pool> pool( new Pool( 40, 256 ) );

    // test common containers and pool overflow
    Test1( pool.get() );

    //
    // test 2
    // 
	
    // allocate a pool of 16384 12-byte elements
    pool.reset( new Pool( 24, 16384 ) );

    // test insertion times vs the standard allocator
    Test2( pool.get() );

    return (0);
}

extern "C" void* old_teststl(void* arg) 
{
    boost::scoped_ptr<Pool> pool ( new Pool(24, 16384) );

    stopwatch_t timer;    

    Test2 (pool.get());

    union {
	double d;
	void* vptr;
    } u = {timer.time()};
    return u.vptr;
}


struct test_thread_t : public thread_t
{
    guard<Pool> m_poolPtr;
    float m_factor;

    test_thread_t(c_str tname, const int granularity, const int elements)
        : thread_t(tname)
    {
        m_poolPtr = new Pool(granularity, elements);
    }

    ~test_thread_t() { }

    void Test()
    {
        
#if 1
        PooledList<int>::Type pooledList( m_poolPtr.get() );
        std::list<int> standardList;
#else
        PooledVec<int>::Type pooledList( m_poolPtr.get() );
        std::vector<int> standardList;
#endif

        clock_t pooledTime = StressList( pooledList );
        clock_t standardTime = StressList( standardList );

        m_factor = float( pooledTime )/float( standardTime );

        //    std::cout << "Relative: " << factor << std::endl;

        //     std::cout << "standard list ticks: " << standardTime << "\n"
        //               << "pooled list ticks: " << pooledTime << "\n"
        //               << "relative time: " << factor << std::endl;
    }


    void work() {
        Test();
    }

}; // EOF test_thread_t


class CObject : private cacheable_iface
{
public:
    CObject() : _kati(-1),_allo(NULL) { 
        cout << " constr\n";
    }
    ~CObject() { 
        cout << " delete ";
        status();
        if (_allo) delete _allo;
    }

    void setup(Pool** stl_pool_list);
    void reset();
    void status();

    int _kati;
    int* _allo;

}; // EOF: CObject

static int counter = 0;


void CObject::setup(Pool** stl_pool_list) {
    _kati++;
    _allo = new int(_kati);
    fprintf( stdout, "(%x) (%d) (%x) (%d)\n", this, _kati, _allo, *_allo);
}

void CObject::reset() {
    assert (_allo);
    fprintf( stdout, "(%x) (%d) (%x) (%d)\n", this, _kati, _allo, *_allo);
    *_allo = 0;
    _kati  = 0;
}

void CObject::status() {
    assert (_allo);
    fprintf( stdout, "(%x) (%d) (%x) (%d)\n", this, _kati, _allo, *_allo);
}

class ippo_cache
{
public:
    guard<object_cache_t<CObject> > cache;
    guard<Pool> pool;
    ippo_cache() {
        Pool** aPool = new Pool*[1];
        pool = new Pool(sizeof(int),1);
        aPool[0] = pool.get();
        cache = new object_cache_t<CObject>(aPool);
    }
    ~ippo_cache() { }
};



// static __thread object_cache_t<CObject>* acache;

// DECLARE_TLS(ippo_cache, hippo);


int main() 
{
    // initialize cordoba threads
    thread_init();

    TRACE_SET( TRACE_ALWAYS | TRACE_STATISTICS 
               //               | TRACE_NETWORK 
               //               | TRACE_CPU_BINDING
               //              | TRACE_QUERY_RESULTS
               //              | TRACE_PACKET_FLOW
               //               | TRACE_RECORD_FLOW
               //               | TRACE_TRX_FLOW
               | TRACE_DEBUG
               );    

    static int const THREADS = 64;
    test_thread_t* testers[THREADS];

    for(int k=1; k <= THREADS; k*=2) {
        TRACE(TRACE_ALWAYS, "%d threads\n", k);
	for(long i=0; i<k; i++) {
            testers[i] = new test_thread_t(c_str("CL-%d",i), 24, 3200);
            assert (testers[i]);
            testers[i]->fork();
        }

        float total_factor = 0;
	for(long i=0; i<k; i++) {
            testers[i]->join();
            total_factor += testers[i]->m_factor;
            TRACE( TRACE_ALWAYS, "Factor: %.2f\n", testers[i]->m_factor);
            delete (testers[i]);
	}
        TRACE( TRACE_ALWAYS, "Threads (%d) - Avg. Factor (%.2f)\n", k, total_factor/k);
    }


//     //    hippo = new ippo_cache();
//     CObject* lala = hippo->cache->borrow();
//     hippo->cache->giveback(lala);
//     //    delete hippo;

//     Pool** stlpools = new Pool*[1];
//     stlpools[0] = new Pool(sizeof(int),1);
//     acache = new object_cache_t<CObject>(stlpools);       

//     //    cout << "starting\n";

//     for (int i=0; i<5; i++) {
//         CObject* pobj = acache->borrow();
//         pobj->_kati--;
//         assert (pobj);
//         acache->giveback(pobj);
//     }

//     delete acache;

    return 0;
}

