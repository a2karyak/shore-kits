/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_iter.h
 *
 *  @brief:  Base class for iterators over certain set of tuples
 *
 *  @author: Mengzhi Wang, April 2001
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#ifndef __SHORE_ITER_H
#define __SHORE_ITER_H

#include "sm_vas.h"
#include "util.h"
#include "stages/tpcc/shore/shore_error.h"


ENTER_NAMESPACE(shore);


/****************************************************************** 
 *  @class: tuple_iter
 *
 *  @brief: Abstract class which is the base for table and index 
 *          scan iterators (tscan - iscan).
 *
 ******************************************************************/

template <class file_desc, class file_scanner>
class tuple_iter_t {
protected:
    ss_m*         _db;
    bool          _opened;  /* whether the init is successful */
    file_desc*    _file;
    file_scanner* _scan;
    
public:
    // Constructor
    tuple_iter_t(ss_m* db, file_desc* file)
        : _db(db), _scan(NULL), _file(file), _opened(false) 
    { 
        assert (db);
    }

    virtual ~tuple_iter_t() { }

    // Access methods
    bool opened() const { return _opened; }


    /* ------------------------- */
    /* --- iteration methods --- */
    /* ------------------------- */

    // virtual w_rc_t open_scan()=0; 

    virtual w_rc_t next(ss_m* db, bool& eof)=0;

    w_rc_t close_scan() {
        if (_opened) delete _scan;
        _opened = false;
        return RCOK;
    }    

}; // EOF: tuple_iter_t



EXIT_NAMESPACE(shore);

#endif /* __SHORE_ITER_H */
