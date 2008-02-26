/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_helper_loader.h
 *
 *  @brief Definition of helper loader thread classes
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __SHORE_HELPER_LOADER_H
#define __SHORE_HELPER_LOADER_H

#include "sm_vas.h"
#include "util.h"

#include "sm/shore/shore_table.h"


ENTER_NAMESPACE(shore);
    
    
/****************************************************************** 
 *
 *  @class: db_init_smt_t
 *
 *  @brief: An smthread inherited class that it is used for initiating
 *          the Shore environment
 *
 ******************************************************************/

class db_init_smt_t : public thread_t 
{
private:
    ShoreEnv* _env;
    int       _rv;

public:
    
    db_init_smt_t(c_str tname, ShoreEnv* db) 
	: thread_t(tname), _env(db)
    {
        assert (_env);
    }

    ~db_init_smt_t() { }

    // thread entrance
    void work();

    inline int rv() { return (_rv); }

}; // EOF: db_init_smt_t


    
/****************************************************************** 
 *
 *  @class: db_load_smt_t
 *
 *  @brief: An smthread inherited class that it is used for loading
 *          the Shore database
 *
 ******************************************************************/

class db_load_smt_t : public thread_t 
{
private:
    ShoreEnv* _env;
    int       _rv;

public:
    
    db_load_smt_t(c_str tname, ShoreEnv* db) 
	: thread_t(tname), _env(db)
    {
        assert (_env);
    }

    ~db_load_smt_t() { }

    // thread entrance
    void work() {
        _rc = _env->loaddata();
        _rv = 0;
    }

    inline int rv() { return (_rv); }
    w_rc_t    _rc;

}; // EOF: db_load_smt_t


    
/****************************************************************** 
 *
 *  @class: table_loading_smt_t
 *
 *  @brief: An smthread inherited class that it is used for spawning
 *          multiple table loading threads. 
 *
 ******************************************************************/

class table_loading_smt_t : public thread_t 
{
private:
    ss_m*         _pssm;
    table_desc_t* _ptable;
    const int     _sf;    
    const char*   _datadir;
    int           _rv;

public:
    
    table_loading_smt_t(c_str tname, ss_m* assm, table_desc_t* atable, 
                        const int asf, const char* adatadir) 
	: thread_t(tname), _pssm(assm), _ptable(atable), _sf(asf),
          _datadir(adatadir)
    {
        assert (_pssm);
        assert (_ptable);
        assert (_sf);
        assert (_datadir);
    }

    ~table_loading_smt_t() { }

    // thread entrance
    void work();

    inline int rv() { return (_rv); }
    inline table_desc_t* table() { return (_ptable); }

}; // EOF: table_loading_smt_t



/****************************************************************** 
 *
 *  @class: index_loading_smt_t
 *
 *  @brief: An smthread inherited class that it is used for helping
 *          the index loading. 
 *
 *  @note:  Thread for helping the index loading. In order to do the
 *          index loading we need to open an iterator on the main table.
 *          Unfortunately, we cannot commit while loading, cause the
 *          iterator closes automatically.
 *
 *
 ******************************************************************/

class index_loading_smt_t : public thread_t 
{
private:
    ss_m*         _pssm;
    table_desc_t* _ptable;
    index_desc_t* _pindex;
    int           _t_count;
    int           _rv;


public:
    
    index_loading_smt_t(c_str tname, ss_m* assm, table_desc_t* aptable,
                        index_desc_t* apindex, table_row_t* aprow) 
	: thread_t(tname), _pssm(assm), _ptable(aptable), _pindex(apindex),
          _t_count(0), _prow(aprow), _has_to_consume(false), 
          _start(false), _finish(false)
    {
        assert (_pssm);
        assert (_ptable);
        assert (_pindex);
        assert (_prow);

        //        pthread_mutex_init(&_cs_mutex, NULL);
    }

    ~index_loading_smt_t() { 
        //        pthread_mutex_destroy(&_cs_mutex);
    }

    inline int rv() { return (_rv); }

    // thread entrance
    void work();

    w_rc_t do_help();
    int    count() { return (_t_count); }

    table_row_t*    _prow;
    mcs_lock      _cs_mutex; /* (?) */
    //    tatas_lock      _cs_mutex; /* (215) */
    //    pthread_mutex_t _cs_mutex; /* (263) */
    bool            _has_to_consume;
    bool            _start;
    bool            _finish;

}; // EOF: index_loading_smt_t



/****************************************************************** 
 *
 *  @class table_checking_smt_t
 *
 *  @brief An smthread inherited class that it is used for spawning
 *         multiple table checking consistency threads. 
 *
 ******************************************************************/

class table_checking_smt_t : public thread_t 
{
private:
    ss_m*         _pssm;
    table_desc_t* _ptable; 

public:
    
    table_checking_smt_t(c_str tname, ss_m* assm, table_desc_t* atable) 
	: thread_t(tname), _pssm(assm), _ptable(atable)
    {
        assert (_pssm);
        assert (_ptable);
    }

    ~table_checking_smt_t() { }

    // thread entrance
    void work();

}; // EOF: table_checking_smt_t



/****************************************************************** 
 *
 *  @class close_smt_t
 *
 *  @brief An smthread inherited class that it is used just for
 *         closing the database.
 *
 ******************************************************************/

class close_smt_t : public thread_t {
private:
    ShoreEnv* _env;    

public:
    int	_rv;
    
    close_smt_t(ShoreEnv* env, c_str tname) 
	: thread_t(tname), 
          _env(env), _rv(0)
    {
    }

    ~close_smt_t() {
    }


    // thread entrance
    void work() {
        assert (_env);
        TRACE( TRACE_ALWAYS, "Closing env...\n");
        if (_env) {
            _env->close();
            delete (_env);
            _env = NULL;
        }        
    }


    /** @note Those two functions should be implemented by every
     *        smthread-inherited class that runs using run_smthread()
     */
    inline int retval() { return (_rv); }
    
}; // EOF: close_smt_t



/****************************************************************** 
 *
 *  @class dump_smt_t
 *
 *  @brief An smthread inherited class that it is used just for
 *         dumping the database.
 *
 ******************************************************************/

class dump_smt_t : public thread_t {
private:
    ShoreEnv* _env;    

public:
    int	_rv;
    
    dump_smt_t(ShoreEnv* env, c_str tname) 
	: thread_t(tname), 
          _env(env), _rv(0)
    {
    }

    ~dump_smt_t() {
    }


    // thread entrance
    void work() {
        assert (_env);
        TRACE( TRACE_DEBUG, "Dumping...\n");
        _env->dump();
        _rv = 0;
    }


    /** @note Those two functions should be implemented by every
     *        smthread-inherited class that runs using run_smthread()
     */
    inline int retval() { return (_rv); }
    
}; // EOF: dump_smt_t



EXIT_NAMESPACE(shore);


#endif /* __SHORE_HELPER_LOADER_H */

