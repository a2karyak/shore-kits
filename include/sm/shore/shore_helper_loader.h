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
    int           _rv;

public:
    
    table_loading_smt_t(c_str tname, ss_m* assm, table_desc_t* atable, 
                        const int asf) 
	: thread_t(tname), _pssm(assm), _ptable(atable), _sf(asf)
    {
        assert (_pssm);
        assert (_ptable);
        assert (_sf);
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
          _t_count(0), _prow(aprow), _has_consumed(true), 
          _start(false), _finish(false)
    {
        assert (_pssm);
        assert (_ptable);
        assert (_pindex);
        assert (_prow);

        pthread_mutex_init(&_cs_mutex, NULL);
    }

    ~index_loading_smt_t() { 
        pthread_mutex_unlock(&_cs_mutex);
    }

    inline int rv() { return (_rv); }

    // thread entrance
    void work();

    w_rc_t do_help();
    int    count() { return (_t_count); }

    table_row_t*    _prow;
    pthread_mutex_t _cs_mutex;
    bool            _has_consumed;
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



EXIT_NAMESPACE(shore);


#endif /* __SHORE_HELPER_LOADER_H */

