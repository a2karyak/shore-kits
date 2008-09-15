/* -*- mode:C++; c-basic-offset:4 -*- */
// CC -m64 -mt -I ~ipandis/apps/berkeleydb/include -I ../../include -g -xdebugformat=stabs -features=extensions -library=stlport4 -lpthread bdb_tpcc_load.cpp   ~ipandis/apps/berkeleydb/lib/libdb_cxx.a -o tpcc_load_bdb
#define NON_EMPTY_CODE

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#ifdef NON_EMPTY_CODE
#include <db_cxx.h>
#include "workload/tpcc/tpcc_tbl_parsers.h"
#endif

#include <stdlib.h>
#include "util/stopwatch.h"
#include <cassert>

#ifdef NON_EMPTY_CODE
using namespace tpcc;

DEFINE_TPCC_PARSER(ORDER) {
    
    // split line into tab separated parts
    record_t record;
    char* lasts;
    char* tmp = strtok_r(linebuffer, "|", &lasts);
    record.first.O_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.first.O_C_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.first.O_D_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.first.O_W_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.O_ENTRY_D = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.O_CARRIER_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.O_OL_CNT = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.O_ALL_LOCAL = atoi(tmp);

    return record;
}


DbEnv* dbe;

// all threads check this regularly to see whether they should quit
// due to another thread failing.
bool volatile error = false;

struct open_file {
    char const* _name;
    FILE* _fd;
    
    open_file(char const* name, char const* flags) : _name(name), _fd(fopen(name, flags)) { }
    operator FILE*() { return _fd; }
    operator char const*() { return _name; }
    ~open_file() { if(_fd) fclose(_fd); }
};

struct insert_thread {
    int _tid;
    open_file _file;
    
    insert_thread(int tid, char const* fname)
	: _tid(tid), _file(fname, "r")
    {
    }
    void run();
    virtual void insert_row(char* linebuffer)=0;
    virtual void commit()=0;
    virtual void rollback()=0;
    virtual ~insert_thread() { }

};

struct bdb_insert_thread : insert_thread {
    // needs to be in scope any time a thread talks to bdb
    Db _db;
    DbTxn* _xct;
    char _dbname[100];
    
    bdb_insert_thread(int tid, char const* fname)
	: insert_thread(tid, fname), _db(dbe, 0)
    {
	// if we ever decide not to use the default lexical compare...
	// _db.set_bt_compare(...)
	
	_db.set_flags(DB_DUPSORT); // allow duplicates, keep them sorted
	
	// create the database table within a trx
	snprintf(_dbname, sizeof(_dbname), "test%02d", tid);
	int flags = DB_AUTO_COMMIT | DB_CREATE | DB_THREAD;
	_db.open(NULL, _dbname, NULL, DB_BTREE, flags, 0);
	// autocommit, file, 1:1 file/db, btree, flags, default perms

	begin();
    }

    ~bdb_insert_thread() {
	_xct->abort();
	_db.close(0);
	dbe->dbremove(NULL, _dbname, NULL, DB_AUTO_COMMIT); // xct, dbfile, dbsubfile, flags
    }
    
    virtual void insert_row(char* linebuffer)=0;
    void begin() {
	dbe->txn_begin(NULL, &_xct, 0); // no parent, handle, default flags (acid, full isolation)
    }
    virtual void commit() {
	_xct->commit(0); // default flags (synchronous flush)
	begin();
    }
    virtual void rollback() {
	_xct->abort();
	begin();
    }
    
};

struct bdb_order_insert_thread : bdb_insert_thread {
    typedef parse_tpcc_ORDER::record_t record_t;
    parse_tpcc_ORDER _parser;

    bdb_order_insert_thread(int tid, char const* fname)
	: bdb_insert_thread(tid, fname)
    {
    }
    virtual void insert_row(char* linebuffer);
};

void bdb_order_insert_thread::insert_row(char* linebuffer) {
    static __thread count = 0;
    record_t record = _parser.parse_row(linebuffer);
    record.second.O_ALL_LOCAL = ++count; // bdb chokes on true duplicates
    Dbt key(&record.first, sizeof(record.first));
    Dbt data(&record.second, sizeof(record.second));
    _db.put(_xct, &key, &data, 0);
}

void insert_thread::run() {
    static int const LOOPS = 2;
    static int const INTERVAL = 1000;
    static int const MAX_LINE_LENGTH = 1000;
    int i=0;
    int mark = INTERVAL;
    char linebuffer[MAX_LINE_LENGTH];
    unsigned long progress = 0;
    for(int j=0; j < LOOPS; j++) {
	fseek(_file, 0, SEEK_SET);
	for(; fgets(linebuffer, MAX_LINE_LENGTH, _file); i++) {
	    insert_row(linebuffer);
	    //	    progress_update(&progress);
	    if(i >= mark) {
		commit();
		mark += INTERVAL;
	    }
	}
    }
    fprintf(stderr, " [%d:%d]", _tid, i);

    // done!
    commit();
}
#endif
static int const ROUNDS = 3;
pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t global_master_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t global_slave_cond = PTHREAD_COND_INITIALIZER;
int global_round = 0;
int global_ready = 0;

int threads;
#ifdef NON_EMPTY_CODE
extern "C" void* run(void* arg) {
    long tid = (long) arg;
    fprintf(stderr, "Thread %d started\n", tid);

    bdb_order_insert_thread t(tid, "tbl_tpcc/order.dat");
    
    for(int i=0; i <= ROUNDS; i++) {
	// tell the master we're ready to start
	pthread_mutex_lock(&global_mutex);
	global_ready++;
	pthread_cond_signal(&global_master_cond);
	pthread_mutex_unlock(&global_mutex);

	if(i == ROUNDS)
	    break;

	// wait for the signal
	pthread_mutex_lock(&global_mutex);
	while(global_round <= i)
	    pthread_cond_wait(&global_slave_cond, &global_mutex);
	pthread_mutex_unlock(&global_mutex);
	
	t.run();
    }
    return NULL;
}
#else
extern "C" void* run(void* arg) { return NULL;}
#endif

static int const THREADS = 50;
int main(int argc, char* argv[]) {
    do {
	if(argc == 2) {
	    threads = atoi(argv[1]);
	    if(threads > 0 && threads <= THREADS)
		break;
	}

	fprintf(stderr, "Usage: ./load_tpcc_bdb <num-threads>\n");
	exit(-1);
    } while(0);
#ifdef NON_EMPTY_CODE
    // set up the database
    DbEnv local_dbe(0);
    int flags = DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_MPOOL
	| DB_INIT_TXN | DB_CREATE | DB_PRIVATE | DB_THREAD;
    local_dbe.set_data_dir("/raid/ryanjohn/bdb-data");
    local_dbe.set_lg_dir("/tmp/tmpfs/ryanjohn/bdb-log");
    local_dbe.set_cachesize(1, 0, 0); // 1.0GB, contiguous
    local_dbe.set_lk_max_locks(1000*threads);
    local_dbe.set_lk_max_objects(1000*threads);
    //    local_dbe.set_lk_max_lockers(1000*threads);
    local_dbe.open(NULL, flags, 0); // no specific home dir, flags, default file perms
    dbe = &local_dbe;
#endif
    // spawn threads
    pthread_t tids[THREADS];
    for(int i=0; i < threads; i++)
	pthread_create(&tids[i], NULL, run, (void*) i);

    for(int i=0; i <= ROUNDS; i++) {
	stopwatch_t timer;
	pthread_mutex_lock(&global_mutex);
	while(global_ready != threads)
	    pthread_cond_wait(&global_master_cond, &global_mutex);
	global_ready = 0;
	global_round++;
	pthread_cond_broadcast(&global_slave_cond);
	pthread_mutex_unlock(&global_mutex);
	
	fprintf(stderr, "\nTotal execution time: %.3lf s\n", timer.time());
    }
    for(int i=0; i < threads; i++)
	pthread_join(tids[i], NULL);

    fprintf(stderr, "Exiting...\n");
#ifdef NON_EMPTY_CODE
    local_dbe.close(0);
#endif
    return 0;
}
