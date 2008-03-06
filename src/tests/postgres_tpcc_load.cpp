/* -*- mode:C++; c-basic-offset:4 -*- */
// CC -mt  -I ../../include -g -xdebugformat=stabs -features=extensions -library=stlport4 -lpthread -lpq postgres_tpcc_load.cpp -o tpcc_load_postgres
#define NON_EMPTY_CODE

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#ifdef NON_EMPTY_CODE
#include <pgsql/libpq-fe.h>
#endif

#include <stdlib.h>
#include "util/stopwatch.h"
#include <cassert>
#include <stdarg.h>

#ifdef NON_EMPTY_CODE




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

struct open_connection {
    PGconn* _conn;
    open_connection(char const* info="dbname=shorecmp")
	: _conn(PQconnectdb(info))
    {
	assert(PQstatus(_conn) == CONNECTION_OK);
    }
    ~open_connection() {
	PQfinish(_conn);
    }
    operator PGconn*() { assert(PQstatus(_conn) == CONNECTION_OK); return _conn; }
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
    open_connection _conn;
    char _tname[100];
    bdb_insert_thread(int tid, char const* fname)
	: insert_thread(tid, fname)
    {
	snprintf(_tname, sizeof(_tname), "test%02d", tid);
	execute("drop table %s", _tname);
	execute("create table %s (O_ID int, O_D_ID int, O_W_ID int, O_C_ID int, "
		"O_ENTRY_D int, O_CARRIER_ID int, O_OL_COUNT int, O_ALL_LOCAL int) ", _tname);
	execute("create index %s_idx on %s (O_ID, O_D_ID, O_W_ID) ", _tname, _tname);
	char stmt[1000];
	snprintf(stmt, sizeof(stmt), "insert into %s values($1, $2, $3, $4, $5, $6, $7, $8)", _tname);
	check(PQprepare(_conn, "pinsert", stmt, 8, NULL));
	begin();
    }

    ~bdb_insert_thread() {
	execute("ABORT");
	execute("drop table test%02d\n", _tid);
    }
    PGresult* check(PGresult* r) {
	assert(r);
	switch(PQresultStatus(r)) {
	case PGRES_TUPLES_OK:
	    return r;
	case PGRES_COMMAND_OK:
	    break;
	default:
	    fprintf(stderr, "Statement\n%s\nfailed with error %s\n", PQresultErrorMessage(r));
	}
	PQclear(r);
	return NULL;
    }
    PGresult* execute(char const* statement, ...) {
	char buffer[1000];
	va_list args;
	va_start(args, statement);
	vsnprintf(buffer, sizeof(buffer), statement, args);
	va_end(args);
	PGresult* r = PQexec(_conn, buffer);
	return check(r);
    }
    virtual void insert_row(char* linebuffer)=0;
    void begin() {
	execute("BEGIN");
    }
    virtual void commit() {
	execute("END");
	begin();
    }
    virtual void rollback() {
	execute("ABORT");
	begin();
    }
    
};

struct bdb_order_insert_thread : bdb_insert_thread {

    bdb_order_insert_thread(int tid, char const* fname)
	: bdb_insert_thread(tid, fname)
    {
    }
    virtual void insert_row(char* linebuffer);
};

void bdb_order_insert_thread::insert_row(char* linebuffer) {
    static __thread count = 0;
    char* state;
    char const* values[] = {
	strtok_r(linebuffer, "|", &state), strtok_r(NULL, "|", &state),
	strtok_r(NULL, "|", &state), strtok_r(NULL, "|", &state),
	strtok_r(NULL, "|", &state), strtok_r(NULL, "|", &state),
	strtok_r(NULL, "|", &state), strtok_r(NULL, "|", &state)
    };
    check(PQexecPrepared(_conn, "pinsert", 8, values, NULL, NULL, 0));
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
static int const ROUNDS = 2;
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
    return 0;
}
