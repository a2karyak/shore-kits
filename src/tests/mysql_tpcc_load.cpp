// -*- mode:c++; c-basic-offset:4 -*-
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <mysql.h>

#include "stopwatch.h"

// Just a small exmaple of multithreading, MUST link with -lpthreads -lmysqlclient_r
// Note: Optimal # of threads and connections pool is the # of CPUs BUT,
// that depends a lot on how fast you expect the answer to your queries

// CC -m64 -mt -L ~/apps/mysql/lib/mysql  -I ~/apps/mysql/include/mysql  -lpthread -lmysqlclient_r  mysql_tpcc_load.cpp -g


#define QPERTHR 500
#define THREADS 32
#define CONPOOL (THREADS)

struct db_config {
    char host[16];
    char user[16];
    char pass[16];
    char name[16];
    unsigned int port;
    char *socket;
};

struct db_mutex {
    MYSQL *db;
    pthread_mutex_t lock;
};

db_mutex dbm[CONPOOL];

int threads;
static int const ROUNDS = 3;
static int const ROWS = 30000;
static int const MAX_LINE_LENGTH = 1000;
static char const* const TABLE_INPUT_FILE  = "tbl_tpcc/order.dat";

pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t global_master_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t global_slave_cond = PTHREAD_COND_INITIALIZER;
int global_round = 0;
int global_ready = 0;

extern "C" void *db_pthread(void *arg);
static void db_die(char const *fmt, ...);
MYSQL *db_connect(MYSQL *db, db_config *dbc);
void db_disconnect(MYSQL *db);
long db_query(MYSQL *db, pthread_mutex_t *lock, const char *query);

int main(int argc, char **argv) {
    int i;
    pthread_t pthread[THREADS];
    db_config dbc;

    do {
	if(argc == 2) {
	    threads = atoi(argv[1]);
	    if(threads > 0 && threads <= THREADS)
		break;
	}

	fprintf(stderr, "Usage: ./load_tpcc_db2 <num-threads>\n");
	exit(-1);
    } while(0);
    
    strcpy(dbc.host,"");
    strcpy(dbc.user,"ryanjohn");
    strcpy(dbc.pass,"");
    strcpy(dbc.name,"ryan1");
    dbc.port = 3306;
    dbc.socket = NULL;

    if (!mysql_thread_safe())
	fprintf(stderr, "Thread Safe OFF\n");
    else
	fprintf(stderr, "Thread Safe ON\n");
    fprintf(stdout, "DB Connections: %d, Threads: %d, Queries per Thread: %d, Total Queries: %d\n",\
	    THREADS, threads, ROWS, threads * ROWS);

    // pre initialize connections and locks
    for (i = 0; i < threads; ++i) {
	dbm[i].db = db_connect(dbm[i].db=NULL, &dbc);
	pthread_mutex_init(&dbm[i].lock, NULL);
    }

    // pthread_setconcurrency(THREADS);
    // fire up the threads
    for (i = 0; i < threads; ++i)
	pthread_create(&pthread[i], NULL, db_pthread, (void *)i);

    // run the benchmark
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
    
    // wait for threads to finish
    for (i = 0; i < threads; ++i)
	pthread_join(pthread[i], NULL);

    for (i = 0; i < threads; ++i) {
	pthread_mutex_destroy(&dbm[i].lock);
	db_disconnect(dbm[i].db);
    }

    exit(EXIT_SUCCESS);
}
struct open_file {
    char const* _name;
    FILE* _fd;
    
    open_file(char const* name, char const* flags) : _name(name), _fd(fopen(name, flags)) { }
    operator FILE*() { return _fd; }
    operator char const*() { return _name; }
    ~open_file() { if(_fd) fclose(_fd); }
};


struct insert_benchmark {
    long _tid;
    MYSQL* _db;
    open_file _file;
    
    insert_benchmark(long tid, char const* fname)
	: _tid(tid), _db(dbm[tid].db), _file(fname, "r")
    {
	char query[1000];
	if(mysql_autocommit(_db, false))
	    db_die("Unable to disable autocommit on thread %ld", _tid);
	
	snprintf(query, sizeof(query), "drop table if exists test%02ld", tid);
	db_query(_db, NULL, query);
	snprintf(query, sizeof(query),
		 "create table test%02ld (O_ID int, O_D_ID int, O_W_ID int, O_C_ID int, "
		 "O_ENTRY_D int, O_CARRIER_ID int, O_OL_COUNT int, O_ALL_LOCAL int) "
		 "ENGINE InnoDB", tid);
	db_query(_db, NULL, query);
	commit();
    }
    void commit() {
	if(mysql_commit(_db))
	    db_die("Commit failed on thread %ld\n", _tid);
    }
    void insert_row(char* linebuffer) {
	char* state;
	char query[1000];
	snprintf(query, sizeof(query),
		 "insert into test%02ld values (%d, %d, %d, %d, %d, %d, %d, %d)", _tid,
		 strtok_r(linebuffer, "|", &state), strtok_r(NULL, "|", &state),
		 strtok_r(NULL, "|", &state), strtok_r(NULL, "|", &state),
		 strtok_r(NULL, "|", &state), strtok_r(NULL, "|", &state),
		 strtok_r(NULL, "|", &state), strtok_r(NULL, "|", &state));
	
	db_query(_db, NULL, query);
    }
    void run() {
	static int const LOOPS = 2;
	static int const INTERVAL = 1000;
	int i=0;
	int mark = INTERVAL;
	char linebuffer[MAX_LINE_LENGTH];
	unsigned long progress = 0;
	for(int j=0; j < LOOPS; j++) {
	    fseek(_file, 0, SEEK_SET);
	    for(; fgets(linebuffer, MAX_LINE_LENGTH, _file); i++) {
		insert_row(linebuffer);
		if(i >= ROWS)
		    break;
		//	    progress_update(&progress);
		if(i >= mark) {
		    commit();
		    mark += INTERVAL;
		}
	    }
	}

	// done!
	commit();
	fprintf(stderr, " [%ld]", _tid, i);
    }
};
void *db_pthread(void *arg) {
    long tid = (long) arg, j;
    int cancelstate;

    // Always a good idea to disable thread cancel state or
    // unexpected crashes may occur in case of database failures
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&cancelstate);
    if ((mysql_thread_init() != 0))
	db_die("mysql_thread_init failed: %s", mysql_error(dbm[tid].db));

    insert_benchmark bmark(tid, TABLE_INPUT_FILE);
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

	// do the actual work...
	bmark.run();
    }
    
    mysql_thread_end();
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,0);
    return NULL;
}

static void db_die(char const *fmt, ...) {
    int i;

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    (void)putc('\n', stderr);
    for (i = 0; i < CONPOOL; ++i) {
	pthread_mutex_destroy(&dbm[i].lock);
	db_disconnect(dbm[i].db);
    }
    exit(EXIT_FAILURE);
}

MYSQL *db_connect(MYSQL *db, db_config *dbc) {
    if (!(db = mysql_init(db)))
	db_die("mysql_init failed: %s", mysql_error(db));
    else {
	if (!mysql_real_connect(db, dbc->host, dbc->user, dbc->pass, dbc->name, dbc->port, dbc->socket, 0))
	    db_die("mysql_real_connect failed: %s", mysql_error(db));
    }
    return (db);
}

void db_disconnect(MYSQL *db) {
    if (db)
	mysql_close(db);
}

long db_query(MYSQL *db, pthread_mutex_t *lock, const char *query) {
    long ret;

    // lock must be called before mysql_query
    //    pthread_mutex_lock(lock);
    ret = mysql_query(db, query);
    // if query failed, exit with db error
    if (ret != 0) {
	// Get rid of the lock first
	//	pthread_mutex_unlock(lock);
	db_die("mysql_query failed: %s", mysql_error(db));
    }
    // if query succeeded
    else {
	MYSQL_RES *res;

	res = mysql_store_result(db);
	// Get rid of the lock ASAP, only safe after mysql_store_result
	//	pthread_mutex_unlock(lock);
	// if there are rows
	if (res) {
	    MYSQL_ROW row, end_row;
	    unsigned int num_fields;

	    num_fields = mysql_num_fields(res);
	    // count total rows * fields and return the value, if SELECT
	    while ( (row = mysql_fetch_row(res)) )
		for (end_row = row + num_fields; row < end_row; ++row)
		    ++ret;
	    mysql_free_result(res);
	}
	// if there are no rows, should there be any ?
	else {
	    // if query was not a SELECT, return with affected rows
	    if (mysql_field_count(db) == 0)
		ret = mysql_affected_rows(db);
	    // there should be data, exit with db error
	    else
		db_die("mysql_store_result failed: %s", mysql_error(db));
	}
    }
    return (ret);
}
