/* -*- mode:C++; c-basic-offset:4 -*- */
// CC -mt  -I ../../include -g -xdebugformat=stabs -features=extensions -library=stlport4 -lpthread -lpq postgres_tpcc_load.cpp -o tpcc_load_postgres

/*
 * PostgreSQL TPC-C kit.
 *
 * Includes a loader and the driver code.
 *
 * NOTE: this file includes itself. Yes, this is a dirty, nasty hack,
 * but it allows the definition and use of prepared statements to stay
 * together, which would be impossible to achieve in a single pass --
 * prepared statements are members of the driver thread, and used
 * inside a member function.
 *
 */
#ifndef NESTED

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <vector>
#include <pgsql/libpq-fe.h>

// can't include this directly, so cut-n-paste instead (grr)
//#include <pgsql/server/catalog/pg_type.h>
#define INT4OID			23
#define INT8OID			20
#define FLOAT4OID 700
#define FLOAT8OID 701
#define BPCHAROID		1042
#define VARCHAROID		1043




#include <stdlib.h>
#include "util/stopwatch.h"
#include <cassert>
#include <stdarg.h>

// should every sql command executed echo to screen?
#undef ECHO_COMMANDS

// should transaction results print?
#define PRINT_RESULTS

inline void throw_if(bool cond, char const* msg) {
    if(cond) throw msg;
}
#define THROW_IF(cond) throw_if(cond, #cond)

#ifdef ECHO_COMMANDS
#define ECHO(format, ...) printf(format, __VA_ARGS__)
#else
#define ECHO(format, ...)
#endif

namespace ryan {
template<class T>
struct delete_policy {
    void operator()(T* ptr) const { delete ptr; }
};

template<class T>
struct array_delete_policy {
    void operator()(T* ptr) const { delete [] ptr; }
};

template<class T>
struct printf_policy {
  void operator()(T* ptr) const {
    if(ptr)
      printf("policy invoked\n");
  }
};
  
// basically an auto_ptr, but allows a user-defined "destructor" policy. 
template<class T, class Policy=delete_policy<T> >
struct auto_ptr {
    struct ref {
	auto_ptr	&_owner;
	explicit ref(auto_ptr &owner) : _auto_ptr(owner) { }
	T* release() const { return _owner.release(); }
    };

    // two constructors so we can make Policy be a reference
    explicit auto_ptr(T* val=NULL) : _val(val) { }
    ~auto_ptr() { clear(); }
    
    auto_ptr(auto_ptr &other) : _val(other.release()) { }
    auto_ptr(ref other) : _val(other.release()) { }

    auto_ptr &operator=(auto_ptr &other) { return assign(other.release()); }
    auto_ptr &operator=(ref other) { return assign(other.release()); }

    T* get() const { return _val; }
    operator T*() const { return get(); }
    T &operator*() const { return *get(); }
    T* operator->() const { return get(); }

    operator ref() { return ref(*this); }
    
    T* release() {
	T* val = get();
	_val = NULL;
	return val;
    }

    void clear() { assign(NULL); }
    auto_ptr &assign(T* new_val) {
	T* val = get();
	if(val != new_val) {
	    Policy()(val);
	    _val = new_val;
	}
	return *this;
    }

    T*		_val;
};
}

// thread-local connection variable. 
static __thread PGconn* my_conn = NULL;
static __thread unsigned rseed;

struct open_connection {
    PGconn* _conn;
    open_connection(char const* info="dbname=shorecmp")
	: _conn(PQconnectdb(info))
    {
	// bind to this thread
	my_conn = _conn;
    }
    ~open_connection() {
	PQfinish(_conn);
    }
    operator PGconn*() { return _conn; }
};

PGresult* execute(char const* statement, ...) {
    char buffer[1000];
    va_list args;
    va_start(args, statement);
    vsnprintf(buffer, sizeof(buffer), statement, args);
    va_end(args);
    ECHO("%s\n", buffer);
    return PQexec(my_conn, buffer);
}


struct result {
    PGresult* _r;
    result(PGresult* r) : _r(r) { }
    ~result() { PQclear(_r); }
    operator PGresult*() { return _r; }
};
#define SPECIALIZE_RESULT(cname, ifq, ifs, ife)		\
    struct cname : result {				\
	    cname(PGresult* r) : result(r) {		\
		THROW_IF(!r);				\
		switch(PQresultStatus(r)) {		\
		case PGRES_TUPLES_OK: 	ifq;		\
		case PGRES_COMMAND_OK:	ifs;		\
		default: 		ife;		\
		}					\
	    }						\
    }

// deallocates the result of a statement, ignoring errors (other than printing them)
SPECIALIZE_RESULT(iresult, break, break, fprintf(stderr, "Ignoring error: %s\n", PQresultErrorMessage(r)));

// deallocates the result of a statement after ensuring that it was
// not an error and did not return results
SPECIALIZE_RESULT(sresult, throw "Statement returned results!", break, throw PQresultErrorMessage(r));

// checks a query result to ensure it was not an error
SPECIALIZE_RESULT(qresult_base, break, throw "Not a query!", throw PQresultErrorMessage(r));

typedef uint64_t int8;
typedef double float8;
typedef char const* str;

struct qresult_col {
    char const* _data;
    int _oid;
    bool _binary;
    qresult_col(PGresult* r, int row, int col)
	: _data(PQgetvalue(r, row, col)), _oid(PQftype(r, col)), _binary(PQfformat(r, col))
    {
    }

#ifdef CHECK_TYPES
#define CHECK(is_true) THROW_IF(!(is_true))
#else
#define CHECK(is_true)
#endif
    char const* check(int oid) { CHECK(_oid == oid); return _data; }
    operator str () { return check(BPCHAROID); }
    operator int () { return *(int*) check(INT4OID); }
    operator int8 () { return *(int8*) check(INT8OID); }
    operator float8 () { return *(float8*) check(FLOAT8OID); }
};
struct qresult : qresult_base {
    int _cur;
    int _rows;
    qresult(PGresult* r) : qresult_base(r), _cur(0), _rows(PQntuples(r)) { }
    bool valid() { return _cur < _rows; }
    qresult &advance(int i=1) { _cur+=i; return *this; } 
    qresult_col get(int i) { THROW_IF(_cur >= _rows); return qresult_col(_r, _cur, i); }
    qresult_col operator[](int i) { return get(i); }
};

struct worker_thread {
    pthread_t _tid;
    open_connection _conn;
    worker_thread()
	: _tid(0)
    {
	THROW_IF(PQstatus(_conn) != CONNECTION_OK);
    }
    
    void fork();
    void join() {
	THROW_IF(!_tid);
	pthread_join(_tid, NULL);
	_tid = 0;
    }
    void fork_join() { fork(); join(); }
    
    virtual ~worker_thread() { }

    virtual void run()=0;
    
    void start() {
	try {
	    rseed = rand(); // seed the RNG for this thread
	    my_conn = _conn; // bind the connection to this thread
	    
	    run();
	} catch(char const* msg) {
	    fprintf(stderr, "Caught error: %s\n", msg);
	}
    }
};
typedef ryan::auto_ptr<worker_thread> worker_thread_p;

extern "C" void* run_worker(void* arg) {
    worker_thread* worker = (worker_thread*) arg;
    worker->start();
    return NULL;
}

void worker_thread::fork() {
    THROW_IF(_tid);
    pthread_create(&_tid, NULL, &run_worker, this);
}

struct active_trx {
    bool _done;
    active_trx() :  _done(false) { sresult sr = execute("BEGIN"); }
    void commit() { _done=true;    iresult ir = execute("END"); }
    ~active_trx() { if(!_done)     iresult ir = execute("ABORT"); }
};


struct index_spec {
    char const* _name;
    char const* _schema;
    bool _unique;
};

struct table_loader : worker_thread  {
    char const* _schema; // table's schema
    char const* _tname; // table's name
    char _fname[100]; // source
    typedef std::vector<index_spec> index_spec_list;
    index_spec_list _index_specs;

    virtual void run();
    
    table_loader(char const* fname, char const* tname, char const* schema)
	: _schema(schema), _tname(tname)
    {
	strlcpy(_fname, fname, sizeof(_fname));
    }
    void add_index(index_spec const &idx) {
	_index_specs.push_back(idx);
    }
};

struct index_loader : worker_thread {
    index_spec _idx;
    table_loader* _owner;
    index_loader(table_loader* owner, index_spec const &idx)
	: _idx(idx), _owner(owner)
    {
    }
    virtual void run();
};

void index_loader::run() {
    char const* tname = _owner->_tname;
    sresult sr = execute("create %s index %s_%s on \"%s\" (%s)",
			 _idx._unique? "unique" : "", tname,
			 _idx._name, tname, _idx._schema);
}

void table_loader::run() {
    
    // drop indexes    
    for(int i=0; i < _index_specs.size(); i++)
	iresult ir = execute("drop index %s_%s", _tname, _index_specs[i]._name);
    
    // drop the table
    {
	iresult ir = execute("drop table \"%s\"", _tname);
    }
    
    // bulk-load the table
    {
	active_trx trx;
	sresult sr1 = execute("create table \"%s\" (%s)", _tname, _schema);
	sresult sr2 = execute("copy \"%s\" from '%s' using delimiters '|'", _tname, _fname);
	trx.commit();
    }

    // create indexes
#if SERIAL
    for(int i=0; i < specs.size(); i++) {
	index_spec &idx = _index_specs[i];
	worker_thread_p(new index_loader(this, idx))->fork_join();
    }
#else
    static int const MAX_IDX_COUNT = 2;
    worker_thread_p threads[MAX_IDX_COUNT]; 
    THROW_IF(_index_specs.size() > MAX_IDX_COUNT);
    for(int i=0; i < _index_specs.size(); i++)  {
	index_spec &idx = _index_specs[i];
	threads[i].assign(new index_loader(this, idx))->fork();
    }
    for(int i=0; i < _index_specs.size(); i++) 
	threads[i]->join();
#endif
}

// a prepared statement wrapper class (binary format)
struct prepared_statement {
    char* _data; // used if a number was passed in
    char const* _name;
    Oid const* _oids;
    int _nparams;
    PGconn* _conn; // cache so it doesn't get changed on us...
    char const* _stmt;
    prepared_statement(char const* name, char const* stmt, Oid const* oids, int nparams)
	: _data(NULL), _name(name), _oids(oids), _nparams(nparams), _conn(my_conn), _stmt(stmt)
    {
	sresult is = PQprepare(_conn, _name, stmt, nparams, oids);
    }
    prepared_statement(char const* name, int i, char const* stmt, Oid const* oids, int nparams)
	: _data(new char[100]), _name(_data), _oids(oids), _nparams(nparams), _conn(my_conn), _stmt(stmt)
    {
	snprintf(_data, 100, "%s_%02d", name, i);
	sresult is = PQprepare(_conn, _name, stmt, nparams, oids);
    }
    PGresult* exec(char const* const* values, int const* sizes, int const* formats) {
	return PQexecPrepared(_conn, _name, _nparams, values, sizes, formats, 1);
	//	return PQexecParams(_conn, _stmt, _nparams, _oids, values, sizes, formats, 1);
    }
    ~prepared_statement() {
	// not allowed to throw in a destructor...
	my_conn = _conn;
	iresult(execute("DEALLOCATE %s", _name)) ; 
	if(_data) delete[] _data;
    }
};

// these are used by the prepared statement macros
template<class T>
int field_len(T const &) { return sizeof(T); }
template<class T>
char const* field_val(T const &val) { return (char const*) &val; }
template<class T>
bool field_bin(T const &) { return true; }

template<>
int field_len(str const& str) { return strlen(str); }
template<>
char const* field_val(str const &val) { return val; }
template<>
bool field_bin(str const &val) { return false; }

// these must all be specified by hand...
template<class T> struct field_oid;
template<> struct field_oid<int> { enum { OID=INT4OID }; };
template<> struct field_oid<int8> { enum { OID=INT8OID }; };
template<> struct field_oid<float8> { enum { OID=FLOAT8OID }; };
template<> struct field_oid<str> { enum { OID=BPCHAROID }; };

/* example:

	DECLARE_PS(foo_sel, 1, int a, field_len(a), field_val(a)) {
		return "select * from foo where ID = $1::int";
	}
*/
#define COMMA ,
#define DECLARE_PS(name, num, oids, args, sizes, vals, formats)		\
    struct name : prepared_statement {					\
	    static char const* query(int idx=0);					\
	    static Oid const* get_oids() { \
		static Oid const o[] = { oids };			\
		return o;						\
	    }								\
	    name() : prepared_statement(#name, query(), get_oids(), num) { } \
	    name(int i) : prepared_statement(#name, i, query(i), get_oids(), num) { } \
	    PGresult* operator()(args) {				\
	      char const* v[] = { vals };				\
	      int const s[] = { sizes };				\
	      int const f[] = { formats };				\
	      return exec(v, s, f);					\
	    }								\
    };									\
    char const* name::query(int i)
#if 0
;
#endif

#define FA(ta, a) ta a
#define FO(ta, a) field_oid<ta>::OID
#define FL(ta, a) field_len(a)
#define FV(ta, a) field_val(a)
#define FB(ta, a) field_bin(a)

#define APPLY_1(macro, a)	\
    macro(a, __a) 
#define DECLARE_PS_1(name,  a)	\
    DECLARE_PS(name, 1, \
	       APPLY_1(FO, a), \
	       APPLY_1(FA, a), \
	       APPLY_1(FL,  a), \
	       APPLY_1(FV,  a), \
	       APPLY_1(FB,  a))

#define APPLY_2(macro, a, b)	\
    macro(a, __a) COMMA macro(b, __b)
#define DECLARE_PS_2(name,  a, b)	\
    DECLARE_PS(name, 2, \
	       APPLY_2(FO,  a, b), \
	       APPLY_2(FA,  a, b), \
	       APPLY_2(FL,  a, b), \
	       APPLY_2(FV,  a, b), \
	       APPLY_2(FB,  a, b))

#define APPLY_3(macro, a, b, c)	\
    macro(a, __a) COMMA macro(b, __b) COMMA macro(c, __c)
#define DECLARE_PS_3(name, a, b, c)	\
    DECLARE_PS(name, 3, \
	       APPLY_3(FO,  a, b, c), \
	       APPLY_3(FA,  a, b, c), \
	       APPLY_3(FL,  a, b, c), \
	       APPLY_3(FV,  a, b, c), \
	       APPLY_3(FB,  a, b, c))

#define APPLY_4(macro, a, b, c, d)					\
    macro(a, __a) COMMA macro(b, __b) COMMA macro(c, __c) COMMA macro(d, __d) 
#define DECLARE_PS_4(name,  a, b, c, d)	\
    DECLARE_PS(name, 4, \
	       APPLY_4(FO,  a, b, c, d), \
	       APPLY_4(FA,  a, b, c, d), \
	       APPLY_4(FL,  a, b, c, d), \
	       APPLY_4(FV,  a, b, c, d), \
	       APPLY_4(FB,  a, b, c, d))

#define APPLY_5(macro, a, b, c, d, e)					\
    macro(a, __a) COMMA macro(b, __b) COMMA macro(c, __c) COMMA macro(d, __d) COMMA \
    macro(e, __e)
#define DECLARE_PS_5(name,  a, b, c, d, e)	\
    DECLARE_PS(name, 5, \
	       APPLY_5(FO,  a, b, c, d, e), \
	       APPLY_5(FA,  a, b, c, d, e), \
	       APPLY_5(FL,  a, b, c, d, e), \
	       APPLY_5(FV,  a, b, c, d, e), \
	       APPLY_5(FB,  a, b, c, d, e))

#define APPLY_6(macro, a, b, c, d, e, f)	\
    macro(a, __a) COMMA macro(b, __b) COMMA macro(c, __c) COMMA macro(d, __d) COMMA \
    macro(e, __e) COMMA macro(f, __f) 
#define DECLARE_PS_6(name,  a, b, c, d, e, f)	\
    DECLARE_PS(name, 6, \
	       APPLY_6(FO,  a, b, c, d, e, f), \
	       APPLY_6(FA,  a, b, c, d, e, f), \
	       APPLY_6(FL,  a, b, c, d, e, f), \
	       APPLY_6(FV,  a, b, c, d, e, f), \
	       APPLY_6(FB,  a, b, c, d, e, f))

#define APPLY_7(macro, a, b, c, d, e, f, g)	\
    macro(a, __a) COMMA macro(b, __b) COMMA macro(c, __c) COMMA macro(d, __d) COMMA \
    macro(e, __e) COMMA macro(f, __f) COMMA macro(g, __g)
#define DECLARE_PS_7(name,  a, b, c, d, e, f, g)	\
    DECLARE_PS(name, 7,					\
	       APPLY_7(FO,  a, b, c, d, e, f, g),	\
	       APPLY_7(FA,  a, b, c, d, e, f, g),	\
	       APPLY_7(FL,  a, b, c, d, e, f, g),	\
	       APPLY_7(FV,  a, b, c, d, e, f, g),	\
	       APPLY_7(FB,  a, b, c, d, e, f, g))

#define APPLY_8(macro, a, b, c, d, e, f, g, h)	\
    macro(a, __a) COMMA macro(b, __b) COMMA macro(c, __c) COMMA macro(d, __d) COMMA \
    macro(e, __e) COMMA macro(f, __f) COMMA macro(g, __g) COMMA macro(h, __h)
#define DECLARE_PS_8(name,  a, b, c, d, e, f, g, h)	\
    DECLARE_PS(name, 8, \
	       APPLY_8(FO,  a, b, c, d, e, f, g, h), \
	       APPLY_8(FA,  a, b, c, d, e, f, g, h), \
	       APPLY_8(FL,  a, b, c, d, e, f, g, h), \
	       APPLY_8(FV,  a, b, c, d, e, f, g, h), \
	       APPLY_8(FB,  a, b, c, d, e, f, g, h))

#define APPLY_9(macro, a, b, c, d, e, f, g, h, i)			\
    macro(a, __a) COMMA macro(b, __b) COMMA macro(c, __c) COMMA macro(d, __d) COMMA \
    macro(e, __e) COMMA macro(f, __f) COMMA macro(g, __g) COMMA macro(h, __h) COMMA \
    macro(i, __i)
#define DECLARE_PS_9(name,  a, b, c, d, e, f, g, h, i)		\
    DECLARE_PS(name, 9,			      	     \
	       APPLY_9(FO,  a, b, c, d, e, f, g, h, i), \
	       APPLY_9(FA,  a, b, c, d, e, f, g, h, i), \
	       APPLY_9(FL,  a, b, c, d, e, f, g, h, i), \
	       APPLY_9(FV,  a, b, c, d, e, f, g, h, i), \
	       APPLY_9(FB,  a, b, c, d, e, f, g, h, i))

#define APPLY_10(macro, a, b, c, d, e, f, g, h, i, j)			\
    macro(a, __a) COMMA macro(b, __b) COMMA macro(c, __c) COMMA macro(d, __d) COMMA \
    macro(e, __e) COMMA macro(f, __f) COMMA macro(g, __g) COMMA macro(h, __h) COMMA \
    macro(i, __i) COMMA macro(j, __j)
#define DECLARE_PS_10(name,  a, b, c, d, e, f, g, h, i, j)		\
    DECLARE_PS(name, 10,			      		\
	       APPLY_10(FO,  a, b, c, d, e, f, g, h, i, j), \
	       APPLY_10(FA,  a, b, c, d, e, f, g, h, i, j), \
	       APPLY_10(FL,  a, b, c, d, e, f, g, h, i, j), \
	       APPLY_10(FV,  a, b, c, d, e, f, g, h, i, j), \
	       APPLY_10(FB,  a, b, c, d, e, f, g, h, i, j))

// collect all the DECLARE_PS_x declarations here so classes can use them below...
#define NESTED
#include "postgres_tpcc.cpp"
#undef NESTED


template<int N>
struct str_field {
    char _data[N+1];
    operator char const*() const { return _data; }
    operator char*() { return _data; }
};

typedef str_field<16> C_NAME;

#define INSTANTIATE_PS(name) name _##name

struct tpcc_payment {
    
    // prepared statements for payment
    INSTANTIATE_PS(c_fetch_for_update_by_id);
    INSTANTIATE_PS(c_fetch_for_update_by_name);
    INSTANTIATE_PS(c_update_totals);
    INSTANTIATE_PS(c_fetch_data_for_update);
    INSTANTIATE_PS(c_update_data);
    INSTANTIATE_PS(d_fetch_for_update);
    INSTANTIATE_PS(d_update_ytd);
    INSTANTIATE_PS(w_fetch_for_update);
    INSTANTIATE_PS(w_update_ytd);
    INSTANTIATE_PS(h_insert);

    // input struct
    struct input {
	int w_id;
	int d_id;

	int c_w_id;
	int c_d_id;
	int c_id; // -1 if by last name
	C_NAME c_last;
    
	float8 h_amount;
	int8 h_date;
    };

    static void format_input(input &in, int sf, int wh);
    void operator()(input &in);
};

struct s_fetch_for_update_pool {
    enum { COUNT=10 };
    long _initialized;// force alignment of _data
    char _data[COUNT*sizeof(s_fetch_for_update)];
    s_fetch_for_update_pool() {
	// use placement-new to initialize these properly
	s_fetch_for_update* array = *this;
	for(_initialized=0; _initialized < COUNT; _initialized++)
	    new (array+_initialized) s_fetch_for_update(_initialized+1);
    }
    ~s_fetch_for_update_pool() {
	// only destruct the ones that we actually constructed...
	s_fetch_for_update* array = *this;
	for(int i=0; i < _initialized; i++) {
	    array[i].~s_fetch_for_update();
	}
    }
    operator s_fetch_for_update*() { return (s_fetch_for_update*) _data; }
};
struct tpcc_new_order {
    INSTANTIATE_PS(c_fetch_info);
    INSTANTIATE_PS(w_fetch_tax);
    INSTANTIATE_PS(d_fetch_for_oid_update);
    INSTANTIATE_PS(d_update_oid);
    INSTANTIATE_PS(o_insert);
    INSTANTIATE_PS(n_insert);
    INSTANTIATE_PS(i_fetch);
    INSTANTIATE_PS(s_update);
    INSTANTIATE_PS(ol_insert);
    
    // 10 s_fetch_for_update, each pointing to a different S_DIST_xx
    s_fetch_for_update_pool _s_fetch_for_update;
    
    struct oline_input {
	int ol_i_id;
	int ol_supply_wh_id; // -1 means home wh
	int ol_qty;
    };
    struct input {
	int w_id;
	int d_id;
	int c_id;
	int ol_cnt;
	oline_input* ol_info;
    };

    static void format_input(input &in, int sf, int wh);
    void operator()(input &in);
};

struct tpcc_thread : worker_thread {
    tpcc_payment _payment;
    tpcc_new_order _new_order;

    enum Transaction { RANDOM=0, PAYMENT, NEW_ORDER};
    int _sf;
    int _wh; // 0 means random
    int _count; // how many trx?
    int _rounds; // how many measurement runs?
    Transaction _which; // -1 means random
    
    // note that the wh is supposed to be fixed for hte life of the
    // client, but we don't have *that* many clients...
    tpcc_thread(int sf, int wh, int count, int rounds, Transaction which=RANDOM)
	: _sf(sf), _wh(wh), _count(count), _rounds(rounds), _which(which) { }
    
    virtual void run();
    
};
static char const* TRX_NAME[] = {"RANDOM", "PAYMENT", "NEW_ORDER"};

// coordination between runs
pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t global_master_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t global_slave_cond = PTHREAD_COND_INITIALIZER;
int global_round = 0;
int global_ready = 0;

void tpcc_thread::run() {
    tpcc_payment::input pin;
    tpcc_new_order::input noin;
    tpcc_new_order::oline_input olin[15];
    noin.ol_info = olin;
    for(int j=0; j <= _rounds; j++) {
	// tell the master we're ready to start
	pthread_mutex_lock(&global_mutex);
	global_ready++;
	pthread_cond_signal(&global_master_cond);
	pthread_mutex_unlock(&global_mutex);

	if(j == _rounds)
	    break;

	// wait for the signal
	pthread_mutex_lock(&global_mutex);
	while(global_round <= j)
	    pthread_cond_wait(&global_slave_cond, &global_mutex);
	pthread_mutex_unlock(&global_mutex);

	// go!
	for(int i=0; i < _count; i++) {
	    try {
		switch(_which) {
		case PAYMENT:
		    tpcc_payment::format_input(pin, _sf, _wh);
		    _payment(pin);
		    break;
		case NEW_ORDER:
		    tpcc_new_order::format_input(noin, _sf, _wh);
		    _new_order(noin);
		    break;
		default:
		    assert(0); // RANDOM not supported yet
		}
	    } catch(str msg) {
		fprintf(stderr, msg); // hopefully just the payment aborting 1% of the time...
	    }
	}
    }
    tpcc_payment::input pi = {1, 1, 1, 1, 1, {""}, 10.58, 3082008};
    //    _payment(pi);
    
    tpcc_new_order::oline_input oli[] = {
	{1, -1, 11},
	{2, -1, 12},
	{3, 3, 13}, // remote wh
	{-1, -1, 14}, // abort!
    };
    enum { NORMAL=2, REMOTE=3, ABORT=4}; 
    tpcc_new_order::input oi = {1, 1, 1, REMOTE, oli};
    _new_order(oi);
}
void tpcc_payment::operator()(tpcc_payment::input &input) {

    // begin transaction...
    active_trx trx;
    
    /*
     * Find the customer, either by C_ID or C_LAST
     */
#else
    DECLARE_PS_3(c_fetch_for_update_by_id, int, int, int) {
	return "select C_LAST, C_FIRST, C_MIDDLE, C_STREET_1, C_STREET_2, "
	    "C_CITY, C_STATE, C_ZIP, C_PHONE, C_SINCE, C_CREDIT, C_CREDIT_LIM, "
	    "C_DISCOUNT, C_BALANCE, C_YTD_PAYMENT, C_PAYMENT_CNT from \"CUSTOMER\" "
	    "where C_W_ID = $1 and C_D_ID = $2 and C_ID = $3 for update";
    }
    DECLARE_PS_3(c_fetch_for_update_by_name, int, int, str) {
	return  "select C_ID, C_FIRST, C_MIDDLE, C_STREET_1, C_STREET_2, "
	    "C_CITY, C_STATE, C_ZIP, C_PHONE, C_SINCE, C_CREDIT, C_CREDIT_LIM, "
	    "C_DISCOUNT, C_BALANCE, C_YTD_PAYMENT, C_PAYMENT_CNT from \"CUSTOMER\" "
	    "where C_W_ID = $1 and C_D_ID = $2 and C_LAST = $3 for update";
    }
#endif
#ifndef NESTED
    qresult c_row = (input.c_id < 0)
	? _c_fetch_for_update_by_name(input.c_w_id, input.c_d_id, input.c_last)
	: _c_fetch_for_update_by_id(input.c_w_id, input.c_d_id, input.c_id);

    if(input.c_id < 0) {
	// need the c_id of the middle customer
	c_row.advance((c_row._rows+1)/2 - 1);
	input.c_id = c_row[0];
    }
    else {
	// need the c_last
	strlcpy(input.c_last, c_row[0], sizeof(input.c_last));
    }


    
    /*
     * Update the customer's totals to reflect the payment
     */
#else
    DECLARE_PS_6(c_update_totals, float8, float8, int, int, int, int) {
	return  "update \"CUSTOMER\" set C_BALANCE = $1, C_YTD_PAYMENT = $2, "
	    "C_PAYMENT_CNT = $3 where C_W_ID = $4 and C_D_ID = $5 and C_ID = $6";
    }
#endif
#ifndef NESTED
    sresult(_c_update_totals((float8) c_row[13] - input.h_amount,
			     (float8) c_row[14] + input.h_amount,
			     (int) c_row[15] + 1,
			     input.c_w_id, input.c_d_id, input.c_id));

    // bad customer?
    char const* c_credit = c_row[10];
    if(c_credit[0] == 'B' && c_credit[1] == 'C') {
	/*
	 * Update data for a bad customer
	 */
#else
	DECLARE_PS_3(c_fetch_data_for_update, int, int, int) {
	    return "select C_DATA_1, C_DATA_2 from \"CUSTOMER\" "
		"where C_W_ID = $1 and C_D_ID = $2 and C_ID = $3 for update";
	}
#endif
#ifndef NESTED
	qresult c_data_row = _c_fetch_data_for_update(input.c_w_id, input.c_d_id, input.c_id);
	str_field<250> data1, data2;
	int count = sprintf(data1, "%d,%d,%d,%d,%d,%1.2f",
			    input.c_id, input.c_d_id, input.c_w_id,
			    input.w_id, input.h_amount);
	strlcpy(data1+count, c_data_row[0], 250-count);

	strncpy(data2, (str) c_data_row[0] + (250-count), count);
	strlcpy(data2+count, c_data_row[1], 250-count);
	
#else
	DECLARE_PS_5(c_update_data, str, str, int, int, int) {
	    return  "update \"CUSTOMER\" set C_DATA_1 = $1, C_DATA_2 = $2 "
		"where C_W_ID = $3 and C_D_ID = $4 and C_ID = $5";
	}
#endif
#ifndef NESTED
	sresult(_c_update_data(data1, data2, input.c_w_id, input.c_d_id, input.c_id));
    }


    
    /*
     * Update the district totals
     */
#else
    DECLARE_PS_2(d_fetch_for_update, int, int) { 
	return "select D_NAME, D_STREET_1, D_STREET_2, D_CITY, D_STATE, D_ZIP, D_YTD "
	    "from \"DISTRICT\" where D_W_ID = $1 and D_ID = $2 for update";
    }
#endif
#ifndef NESTED
    qresult d_row = _d_fetch_for_update(input.w_id, input.d_id);
#else
    DECLARE_PS_3(d_update_ytd, float8, int, int) {
	return "update \"DISTRICT\" set D_YTD = $1 where D_W_ID = $2 and D_ID = $3";
    }
#endif
#ifndef NESTED
    sresult(_d_update_ytd((float8) d_row[6] + input.h_amount, input.w_id, input.d_id));


    
    /*
     * Update the warehouse totals
     */
#else
    DECLARE_PS_1(w_fetch_for_update, int) { 
	return "select W_NAME, W_STREET_1, W_STREET_2, W_CITY, W_STATE, W_ZIP, W_YTD"
	    " from \"WAREHOUSE\" where W_ID = $1 for update";
    }
#endif
#ifndef NESTED
    qresult w_row = _w_fetch_for_update(input.w_id);

#else
    DECLARE_PS_2(w_update_ytd, float8, int) {
	return "update \"WAREHOUSE\" set W_YTD = $1 where W_ID = $2";
    }
#endif
#ifndef NESTED
    sresult(_w_update_ytd((float8) w_row[6] + input.h_amount, input.w_id));
    
    str_field<24> h_data;
    snprintf(h_data, sizeof(h_data), "%s    %s", (char const*) w_row[0], (char const*) d_row[0]);

#warning TODO
    int8 now = 0;


    /*
     * Insert a record into the history table
     */
#else
    DECLARE_PS_8(h_insert, int, int, int, int, int, int, int8, str) {
	return "insert into \"HISTORY\" values($1, $2, $3, $4, $5, $6, $7, $8)";
    }
#endif
#ifndef NESTED
    sresult(_h_insert(input.c_id, input.c_d_id, input.c_w_id, input.d_id,
		      input.w_id, now, input.h_amount, h_data));

    
    // made it!
    trx.commit();

#ifdef PRINT_RESULTS
    
#endif
}

void tpcc_new_order::operator()(tpcc_new_order::input &input) {
    active_trx trx;


    /*
     * Retrieve the customer's info
     */
#else
    DECLARE_PS_3(c_fetch_info, int, int, int) {
	return "select C_DISCOUNT, C_CREDIT, C_LAST from \"CUSTOMER\" "
	    "where C_W_ID = $1 and C_D_ID = $2 and C_ID = $3";
    }
#endif
#ifndef NESTED
    qresult c_row = _c_fetch_info(input.w_id, input.d_id, input.c_id);
    

    
    /*
     * Retrieve W_TAX from the WAREHOUSE
     */
#else
    DECLARE_PS_1(w_fetch_tax, int) {
	return "select W_TAX from \"WAREHOUSE\" where W_ID = $1";
    }
#endif
#ifndef NESTED
    float8 w_tax = qresult(_w_fetch_tax(input.w_id))[0];



    /*
     * Retrieve D_TAX and update D_NEXT_O_ID
     */
#else
    DECLARE_PS_2(d_fetch_for_oid_update, int, int) {
	return "select D_TAX, D_NEXT_O_ID from \"DISTRICT\" "
	    "where D_W_ID = $1 and D_ID = $2 for update";
    }
    DECLARE_PS_2(d_update_oid, int, int) {
	return "update \"DISTRICT\" set D_NEXT_O_ID = D_NEXT_O_ID+1 where D_W_ID = $1 and D_ID = $2";
    }
#endif
#ifndef NESTED
    qresult d_row = _d_fetch_for_oid_update(input.w_id, input.d_id);
    float8 d_tax = d_row[0];
    int o_id = d_row[1];
    sresult(_d_update_oid(input.w_id, input.d_id));
    
    

    /*
     * Insert an ORDER  and NEW_ORDER
     */
#else
    DECLARE_PS_7(o_insert, int, int, int, int, int8, int, int) {
	return "insert into \"ORDER\"(O_ID, O_C_ID, O_D_ID, O_W_ID, O_ENTRY_D, "
	    "O_OL_CNT, O_ALL_LOCAL) values($1,$2,$3,$4,$5,$6,$7)";
    }
    DECLARE_PS_3(n_insert, int, int, int) {
	return "insert into \"NEW_ORDER\" values($1, $2, $3)";
    }
#endif
#ifndef NESTED
    #warning TODO
    int8 now = 0;
    bool all_local = true;
    for(int i=0; i < input.ol_cnt; i++)
	all_local &= (input.ol_info[i].ol_supply_wh_id < 0);    
    sresult(_o_insert(o_id, input.c_id, input.d_id, input.w_id, now, input.ol_cnt, all_local));
    sresult(_n_insert(o_id, input.d_id, input.w_id));


    
    /*
     * Iterate over the line items
     */
    double total_amt = 0;
    for(int i=0; i < input.ol_cnt; i++) {
	oline_input &item = input.ol_info[i];


	/*
	 * Probe the ITEM table. 
	 */
#else
	DECLARE_PS_1(i_fetch, int) {
	    return "select I_PRICE, I_NAME, I_DATA from \"ITEM\" where I_ID = $1";
	}
#endif
#ifndef NESTED
	qresult i_row = _i_fetch(item.ol_i_id);
	THROW_IF(i_row._rows == 0); // rbk!

	

	/*
	 * Update the STOCK table
	 */
#else
#define Q(i) "select S_QUANTITY, S_DIST_" #i ", S_DATA " \
		     "from \"STOCK\" where S_I_ID = $1 and S_W_ID = $2 for update"
	
	// return a different query() every time it's invoked within a particular thread
	DECLARE_PS_2(s_fetch_for_update, int, int) {	\
	    static char const* queries[] = { \
		Q(01), Q(02), Q(03), Q(04), Q(05),			\
		Q(06),Q(07),Q(08),Q(09),Q(10)				\
	    };								\
	    THROW_IF(i > 10 || i < 1);					\
	    return queries[i-1];
	}
#undef Q
	DECLARE_PS_5(s_update, int, int, int, int, int) {
	    return "update \"STOCK\" set S_QUANTITY = $1, S_YTD = S_YTD+$2, "
		"S_ORDER_CNT = S_ORDER_CNT+1, S_REMOTE_CNT = S_REMOTE_CNT+$3 "
		"where S_I_ID = $4 and S_W_ID = $5";
	}
#endif
#ifndef NESTED
	bool local = (item.ol_supply_wh_id < 0);
	int w_id = local? input.w_id : item.ol_supply_wh_id;
	// remember the array is zero-based idx, district id is one-based
	qresult s_row = _s_fetch_for_update[input.d_id-1](item.ol_i_id, w_id);
	int qty = s_row[0];
	int new_qty = qty - item.ol_qty;
	if(new_qty < 10) new_qty += 91;
	sresult(_s_update(new_qty, qty, (local? 0 : 1), item.ol_i_id, w_id));


	
	/*
	 * Insert an ORDERLINE
	 */
#else
	DECLARE_PS_9(ol_insert, int,int,int,int,int,int,int,float8,str) {
	    return "insert into \"ORDERLINE\" (OL_O_ID, OL_D_ID, OL_W_ID, OL_NUMBER, "
		"OL_I_ID, OL_SUPPLY_W_ID, OL_QUANTITY, OL_AMOUNT, OL_DIST_INFO) "
		"values ($1,$2,$3,$4,$5,$6,$7,$8,$9)";
	}
#endif
#ifndef NESTED
	float8 amt = item.ol_qty * (float8) i_row[0];
	total_amt += amt;
	sresult(_ol_insert(o_id, input.d_id, input.w_id, i,
			   item.ol_i_id, w_id, item.ol_qty, amt, s_row[1]));
    }


    // done!
    trx.commit();    
    total_amt *= (1 - (double) c_row[0])*(1 + w_tax + d_tax);    
}

struct tpcc_loader : worker_thread {    
    char const* _data_dir;
    tpcc_loader(char const* data_dir) : _data_dir(data_dir) { }
    table_loader* create_loader(char const* tname, char const* schema) {
	char fname[100];
	snprintf(fname, sizeof(fname), "%s/%s.dat", _data_dir, tname);
	return new table_loader(fname, tname, schema);	
    }

    virtual void run();
};
void tpcc_loader::run() {
    static int const TABLES = 9;
    static char const* const tnames[] = {
	"WAREHOUSE",
	"DISTRICT",
	"CUSTOMER",
	"HISTORY",
	"NEW_ORDER",
	"ORDER",
	"ORDERLINE",
	"ITEM",
	"STOCK"
    };
    static char const* const schemas[] = {
	"W_ID int, W_NAME char(10), W_STREET_1 char(20), W_STREET_2 char(20), "
	"W_CITY char(20), W_STATE char(2), W_ZIP char(9), W_TAX float8, W_YTD float8",

	"D_ID int, D_W_ID int, D_NAME char(10), D_STREET_1 char(20), "
	"D_STREET_2 char(20), D_CITY char(20), D_STATE char(2), D_ZIP char(9), "
	"D_TAX float8, D_YTD float8, D_NEXT_O_ID int",

	"C_ID int, C_D_ID int, C_W_ID int, C_FIRST char(16), C_MIDDLE char(2), "
	"C_LAST char(16), C_STREET_1 char(20), C_STREET_2 char(20), C_CITY char(20), "
	"C_STATE char(2), C_ZIP char(9), C_PHONE char(16), C_SINCE int8, C_CREDIT char(2), "
	"C_CREDIT_LIM float8, C_DISCOUNT float8, C_BALANCE float8, C_YTD_PAYMENT float8, "
	"C_LAST_PAYMENT float8, C_PAYMENT_CNT int, C_DATA_1 char(250), C_DATA_2 char(250)",

	"H_C_ID int, H_C_D_ID int, H_C_W_ID int, H_D_ID int, H_W_ID int, H_DATE int8, "
	"H_AMOUNT float8, H_DATA char(24)",

	"NO_O_ID int, NO_D_ID int, NO_W_ID int",

	"O_ID int, O_C_ID int, O_D_ID int, O_W_ID int, O_ENTRY_D int8, O_CARRIER_ID int, "
	"O_OL_CNT int, O_ALL_LOCAL int",

	"OL_O_ID int, OL_D_ID int, OL_W_ID int, OL_NUMBER int, OL_I_ID int, OL_SUPPLY_W_ID int, "
	"OL_DELIVERY_D int8, OL_QUANTITY int, OL_AMOUNT float8, OL_DIST_INFO char(24)",

	"I_ID int, I_IM_ID int, I_NAME char(24), I_PRICE float8, I_DATA char(50)",

	"S_I_ID int, S_W_ID int, S_REMOTE_CNT int, S_QUANTITY int, S_ORDER_CNT int, "
	"S_YTD int, S_DIST_01 char(24), S_DIST_02 char(24), S_DIST_03 char(24), "
	"S_DIST_04 char(24), S_DIST_05 char(24), S_DIST_06 char(24), S_DIST_07 char(24), "
	"S_DIST_08 char(24), S_DIST_09 char(24), S_DIST_10 char(24), S_DATA char(50)"
    };
    static index_spec const pkeys[] = {
	{"PK", "W_ID", true},
	{"PK", "D_W_ID, D_ID", true},
	{"PK", "C_W_ID, C_D_ID, C_ID", true},
	{NULL, NULL, false},
	{"PK", "NO_W_ID, NO_D_ID, NO_O_ID", true},
	{"PK", "O_W_ID, O_D_ID, O_ID", true},
	{"PK", "OL_W_ID, OL_D_ID, OL_O_ID, OL_NUMBER", true},
	{"PK", "I_ID", true},
	{"PK", "S_W_ID, S_I_ID", true},
    };
	
    ryan::auto_ptr<table_loader> workers[TABLES];
    for(int i=0; i < TABLES; i++) {
	workers[i].assign(create_loader(tnames[i], schemas[i]));
	if(pkeys[i]._name)
	    workers[i]->add_index(pkeys[i]);
    }

    // add the secondary indexes
    index_spec cust2 = {"NAME_IDX", "C_W_ID, C_D_ID, C_LAST, C_FIRST, C_ID", false};
    index_spec order2 = {"CUST_IDX", "O_W_ID, O_D_ID, O_C_ID, O_ID", true};
    workers[2]->add_index(cust2);
    workers[5]->add_index(order2);

#if SERIAL
    for(int i=0; i < TABLES; i++) 
	workers[i]->fork_join();
#else	
    for(int i=0; i < TABLES; i++)
	workers[i]->fork();
    for(int i=0; i < TABLES; i++)
	workers[i]->join();
#endif
}

static int const THREADS = 50;
int main(int argc, char* argv[]) {
    stopwatch_t timer;
    if(argc < 3) {
	printf("usage: ./postgres_tpcc <sf> <wh> <spread?> <clients> <count> <rounds> <trx>\n");
	return -1;
    }
    struct args {
	int sf;
	int wh;
	int spread;
	int clients;
	int count;
	int rounds;
	tpcc_thread::Transaction trx;
    };
    union { args a; int i[7]; } u;
    for(int i=1; i < argc; i++) 
	u.i[i-1] = atoi(argv[i]);
    
    args &a = u.a;

    static int const MAX_CLIENTS = 50;
    // sf
    if(a.sf < 1 || a.sf > 1000) a.sf = 10;
    // wh queried
    if(a.wh < 1 || a.wh > a.sf) a.wh = a.sf;
    // spread?
    if(a.spread != 1) a.spread = 0;
    // clients?
    if(a.clients < 1 || a.clients > MAX_CLIENTS) a.clients = 10;
    // count
    if(a.count < 1 || a.count > 10000) a.count = 100;
    // rounds
    if(a.rounds < 1 || a.rounds > 100) a.rounds = 5;
    if(a.trx < tpcc_thread::PAYMENT || a.trx > tpcc_thread::NEW_ORDER) a.trx = tpcc_thread::PAYMENT;


    printf("TPCC Run: sf=%d, wh=%d, spread=%s, clients=%d, count=%d, rounds=%d, trx=%s\n",
	   a.sf, a.wh, a.spread? "yes" : "no", a.clients, a.count, a.rounds, TRX_NAME[a.trx]);
    
    // seed the global RNG
    srand((int) timer.clock_us());

#if 0
    // loader...
    char const* dir = "/export/home/ryanjohn/projects/qpipe-tpcc-temp/stagetrxdata/sf10";
    tpcc_loader(dir).start();
    printf("Finished loading %s in %d seconds", dir, (int) timer.time());
    return 0;
#else
	// benchmark run
    // spawn threads
    ryan::auto_ptr<tpcc_thread> threads[MAX_CLIENTS];
    for(int i=0; i < a.clients; i++)
	threads[i].assign(new tpcc_thread(a.wh, a.spread? ((i%a.wh)+1) : 0, a.count, a.rounds, a.trx))->fork();

    for(int i=0; i <= a.rounds; i++) {
	stopwatch_t timer;
	pthread_mutex_lock(&global_mutex);
	while(global_ready != a.clients)
	    pthread_cond_wait(&global_master_cond, &global_mutex);
	global_ready = 0;
	global_round++;
	pthread_cond_broadcast(&global_slave_cond);
	pthread_mutex_unlock(&global_mutex);

	double time = timer.time();
	if(i == 0) {
	    fprintf(stderr, "Initialization completed in %.3lf s. Starting measurements\n", time);
	}
	else {
	    int count = a.clients* a.count;
	    fprintf(stderr, "\nTotals:\n"
		    "\tThreads: %d\n"
		    "\tTransactions: %d (%d per thread)\n"
		    "\tWall Time: %.3lf sec\n"
		    "\tThroughput: %.1lf tps\n",
		    a.clients, count, a.count, time, count/time);
	}
    }
    for(int i=0; i < a.clients; i++)
	threads[i]->join();

#endif
    fprintf(stderr, "Exiting...\n");
    return 0;
}


// Input generation functions



    /**
     * Returns a pseudorandom, uniformly distributed int value between
     * 0 (inclusive) and the specified value (exclusive).
     *
     * Source http://java.sun.com/j2se/1.5.0/docs/api/java/util/Random.html#nextInt(int)
     */
int rand(int n) {
    assert(n > 0);

    if ((n & -n) == n)  // i.e., n is a power of 2
	return (int)((n * (uint64_t)rand_r(&rseed)) / (RAND_MAX+1));

    int bits, val;
    do {
	bits = rand();
	val = bits % n;
    } while(bits - val + (n-1) < 0);
        
    return val;
}

int URand(int low, int high) {
    int d = high - low + 1;
    return low + rand(d);
}

int NURand(int a, int low, int high) {
    return (((URand(0, a) | URand(low, high)) + URand(0, a)) % (high-low+1)) + low;
}

char* CUST_LAST[10] = { "BAR", "OUGHT", "ABLE", "PRI", "PRES", "ESE",
                        "ANTI", "CALLY", "ATION", "EING" };


int generate_cust_last(int select, char* dest) 
{  
  assert ((select>=0) && (select<1000));
  assert (dest);

  int i1, i2, i3;

  i3 = select % 10;
  i2 = ((select % 100) - i3) / 10;
  i1 = (select - (select % 100)) / 100;

  int iLen = snprintf(dest, 16, "%s%s%s", 
		      CUST_LAST[i1],
		      CUST_LAST[i2],
		      CUST_LAST[i3]);
  dest[16] = '\0'; // C_LAST is char[16]
  
  return (iLen);
}

void tpcc_new_order::format_input(tpcc_new_order::input &noin, int sf, int specificWH) {
    noin.w_id = specificWH? specificWH : URand(1, sf);
    noin.d_id   = URand(1, 10);
    noin.c_id   = NURand(1023, 1, 3000);
    noin.ol_cnt = URand(5, 15);

    // generate the info order
    for (int i=0; i<noin.ol_cnt; i++) {
        noin.ol_info[i].ol_i_id = NURand(8191, 1, 100000);
        noin.ol_info[i].ol_qty = URand(1, 10);

	bool remote = URand(1, 100) == 1;
	noin.ol_info[i].ol_supply_wh_id = remote? URand(1, sf) : -1;
    }
    
    int rbk    = URand(2, 100); /* if rbk == 1 - ROLLBACK */
    if(rbk == 1)
	noin.ol_info[noin.ol_cnt-1].ol_i_id = -1; // not found...
}

void tpcc_payment::format_input(tpcc_payment::input &pin, int sf, int specificWH) {
    pin.w_id = specificWH? specificWH : URand(1, sf);
    pin.d_id = URand(1, 10);    
    pin.h_amount = (long)URand(100, 500000)/(long)100.00;
    
    bool local = URand(1, 100) <= 85; // 85 - 15
    pin.c_w_id = local? pin.w_id : URand(1, sf);
    pin.c_d_id = local? pin.d_id : URand(1, 10);

    bool by_name = URand(61, 100) <= 60;
    if(by_name) {
	generate_cust_last(NURand(255,0,999), pin.c_last);
	pin.c_id = -1;
    }
    else {
	pin.c_last[0] = 0;
	pin.c_id = NURand(1023, 1, 3000);
    }
}
#endif // !NESTED
