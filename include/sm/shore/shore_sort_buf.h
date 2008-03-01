/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_sort_buf.h
 *
 *  @brief:  In-memory sort buffer structure
 *
 *
 *  @author: Mengzhi Wang, April 2001
 *  @author: Ippokratis Pandis, January 2008
 */

#ifndef   __SHORE_SORT_BUF_H
#define   __SHORE_SORT_BUF_H

#include "sm/shore/shore_iter.h"
#include "sm/shore/shore_table.h"


ENTER_NAMESPACE(shore);


#define   MIN_TUPLES_FOR_SORT     100



/**********************************************************************
 * 
 * Comparison functions
 *
 * @input:  obj1, obj2 of the same type, casted to (const void*)
 *
 * @return: 1 iff obj1>obj2, 0 iff obj1==obj2, -1 iff obj1<obj2
 * 
 * @note:   Currently only INT and SMALLINT types supported (both fixed 
 *          legnth)
 *
 **********************************************************************/


int compare_smallint(const void * d1, const void * d2)
{
    short data1 = *((short*)d1);
    short data2 = *((short*)d2);
    if (data1 > data2) return 1;
    if (data1 == data2) return 0;
    return -1;
}

int  compare_int(const void * d1, const void * d2)
{
    int data1 = *((int*)d1);
    int data2 = *((int*)d2);
    if (data1 > data2) return 1;
    if (data1 == data2) return 0;
    return -1;
}



/**********************************************************************
 *
 * This file contains the in-memory sort buffer structure definition.
 * The sort buffer is defined as a subclass of shore_table (defined in
 * shore_table.h) to take advantage of the schema and tuple value
 * operations. The data waiting to be sorted is stored in a memory
 * buffer (sort_buffer_t::_sort_buf).
 *
 * @note: To simplify the memory management, the sort buffer only works
 *        on fixed length fields.
 *
 **********************************************************************/

class sort_iter_impl;


/**********************************************************************
 *
 * @class:   sort_buffer
 *
 * @warning: NO THREAD-SAFE the caller should make sure that only a 
 *           thread is accessing objects of this class
 *
 **********************************************************************/

class sort_buffer_t : public table_desc_t 
{
    friend class sort_iter_impl;

protected:
    char*       _sort_buf;     /* memory buffer */
    int         _tuple_size;   /* tuple size */
    int         _tuple_count;  /* # of tuples in buffer */
    int         _buf_size;     /* size of the buffer (in # of tuples) */
    bool        _is_sorted;    /* shows if sorted */
    tatas_lock  _sorted_lock; 

    rep_row_t*  _preprow;      /* used for the tuple->format() */

    /* count _tuple_size and allocate buffer */
    void init();

    /* retrieve a tuple */
    bool get_sorted(const int index, table_row_t* ptuple); 

public:

    sort_buffer_t(int field_count, rep_row_t* aprow)
        : table_desc_t("SORT_BUF", field_count), _sort_buf(NULL),
          _tuple_size(0), _tuple_count(0), _buf_size(0), _preprow(aprow),
          _is_sorted(false)
    { 
    }

    ~sort_buffer_t() 
    { 
        if (_sort_buf)
            delete [] _sort_buf;
    }

    /* set the schema - accepts only fixed length */
    void setup(const int index, sqltype_t type, const int len = 0) 
    {
        assert((index>=0) && (index<_field_count));
        _desc[index].setup(type, "", len);
        assert(!_desc[index].is_variable_length());
        assert(!_desc[index].allow_null());
    }

    /* add current tuple to the sort buffer */
    void   add_tuple(table_row_t& atuple);

    /* sort tuples on the first field value */
    void   sort();
    w_rc_t get_sort_iter(ss_m* db, sort_iter_impl* &sort_iter);


    /* needed by table_desc_t in order not to be abstract */
    bool read_tuple_from_line(table_row_t& , char* ) {
        assert (false); // should not be called'
        return (false);
    }

    inline int count() { return (_tuple_count); }

}; // EOF: sort_buffer_t



/**********************************************************************
 *
 * @class: sort_iter_impl
 *
 * @brief: Iterator over a sorted buffer
 *
 * @note:  Iterator that does not need a db handle, since the sorting
 *         takes place only in memory
 *
 **********************************************************************/

typedef tuple_iter_t<sort_buffer_t, int, table_row_t> sort_scan_t;

class sort_iter_impl : public sort_scan_t {
private:
    int _index;

public:

    sort_iter_impl(ss_m* db, sort_buffer_t* psortbuf) 
        : tuple_iter_t(db, psortbuf), _index(0)
    { 
        assert (_file);
        W_COERCE(open_scan());
    }


    /* ------------------------------ */
    /* --- sorted iter operations --- */
    /* ------------------------------ */

    w_rc_t open_scan();
    w_rc_t close_scan() { return (RCOK); };
  
    w_rc_t next(ss_m* db, bool& eof, table_row_t& tuple);

}; // EOF: sort_iter_impl



/**********************************************************************
 *
 * sort_buffer_t methods
 *
 **********************************************************************/


/********************************************************************* 
 *
 *  @fn:    init
 *  
 *  @brief: Calculates the tuple size and allocates the initial memory
 *          buffer for the tuples.
 *
 *********************************************************************/

inline void sort_buffer_t::init()
{
    /* calculate tuple size */
    _tuple_size = 0;
    for (int i=0; i<_field_count; i++)
        _tuple_size += _desc[i].fieldmaxsize();

    /* allocate size for MIN_TUPLES_FOR_SORT tuples */
    _sort_buf = new char[MIN_TUPLES_FOR_SORT*_tuple_size]; 
    _buf_size = MIN_TUPLES_FOR_SORT;

    _is_sorted = false;
}



/********************************************************************* 
 *
 *  @fn:    add_tuple
 *  
 *  @brief: Inserts a new tuple in the buffer. If there is not enough
 *          space, it doubles the allocated space.
 *
 *********************************************************************/

inline void sort_buffer_t::add_tuple(table_row_t& atuple)
{
    CRITICAL_SECTION(cs, _sorted_lock);

    /* setup the tuple size */
    if (!_tuple_size) init();

    /* check if more space is needed */
    if (_buf_size == _tuple_count) {
        /* double the buffer size if needed */
        char* tmp = new char[(2*_buf_size)*_tuple_size];
        memcpy(tmp, _sort_buf, _buf_size*_tuple_size);
        delete [] _sort_buf;

        _sort_buf = tmp;
        _buf_size *= 2;
    }

    /* add the current tuple to the end of the buffer */
    atuple.format(*_preprow);
    assert (_preprow->_dest);
    memcpy(_sort_buf+(_tuple_count*_tuple_size), _preprow->_dest, _tuple_size);
    _tuple_count++;
    _is_sorted = false;    
}



/********************************************************************* 
 *
 *  @fn:   sort
 *  
 *  @note: Uses the old qsort // TODO: use sort() 
 *
 *********************************************************************/

inline void sort_buffer_t::sort()
{
    CRITICAL_SECTION(cs, _sorted_lock);

    /* compute the number of bytes used in sorting */
#if 0
    // displays buffer before/after sorting
    cout << "Before sorting: " << endl;
    if (_desc[0].type() == SQL_SMALLINT) {
        for (int i=0; i<_tuple_count; i++) {
            cout << ((short*)(_sort_buf+i*_tuple_size))[0] << " ";
        }
        else if (_desc[0].type() == SQL_INT) {
            for (int i=0; i<_tuple_count; i++) {
                cout << ((int*)(_sort_buf+i*_tuple_size))[0] << " ";
            }
        }
        cout << endl;
    }
#endif

    // does the sorting
    switch (_desc[0].type()) {
    case SQL_SMALLINT:
        qsort(_sort_buf, _tuple_count, _tuple_size, compare_smallint); break;
    case SQL_INT:
        qsort(_sort_buf, _tuple_count, _tuple_size, compare_int); break;
    default: 
        /* not implemented yet */
        assert(false);
    }
    _is_sorted = true;

#if 0
    cout << "After sorting: " << endl;
    if (_desc[0].type() == SQL_SMALLINT) {
        for (int i=0; i<_tuple_count; i++) {
            cout << ((short*)(_sort_buf+i*_tuple_size))[0] << " ";
        }
        else if (_desc[0].type() == SQL_INT) {
            for (int i=0; i<_tuple_count; i++) {
                cout << ((int*)(_sort_buf+i*_tuple_size))[0] << " ";
            }
        }
        cout << endl;
    }
#endif
}


     
/********************************************************************* 
 *
 *  @fn:    get_sort_iter
 *  
 *  @brief: Initializes a sort_scan_impl for the particular sorter buffer
 *
 *  @note:  It is responsibility of the caller to de-allocate
 *
 *********************************************************************/

inline w_rc_t sort_buffer_t::get_sort_iter(ss_m* db,
                                           sort_iter_impl* &sort_iter)
{
    sort_iter = new sort_iter_impl(db, this);
    return (RCOK);
}
     


/********************************************************************* 
 *
 *  @fn:    get_sorted
 *  
 *  @brief: Returns the tuple in the buffer pointed by the index
 *
 *  @note:  It asserts if the buffer is not sorted
 *
 *********************************************************************/

inline bool sort_buffer_t::get_sorted(const int index, table_row_t* ptuple)
{
    CRITICAL_SECTION(cs, _sorted_lock);

    if (_is_sorted) {
        if (index >=0 && index < _tuple_count) {
            return (ptuple->load(_sort_buf + (index*_tuple_size)));
        }

        TRACE( TRACE_DEBUG, "out of bounds index...\n");
        return (false);
    }

    TRACE( TRACE_DEBUG, "buffer not sorted yet...\n");
    return (false);
}


/**********************************************************************
 *
 * sort_iter_impl methods
 *
 **********************************************************************/



/********************************************************************* 
 *
 *  @fn:    open_scan
 *  
 *  @brief: Opens a scan operator
 *
 *  @note:  If the sorted buffer is not sorted, it sorts it
 *
 *********************************************************************/

inline w_rc_t sort_iter_impl::open_scan()
{ 
    assert (_file);
    assert (_file->_field_count>0);

    _file->sort();

    _index = 0;
    _opened = true;
    return (RCOK);
}


/********************************************************************* 
 *
 *  @fn:    next
 *  
 *  @brief: Gets the next tuple pointed by the index
 *
 *********************************************************************/

inline w_rc_t sort_iter_impl::next(ss_m* db, bool& eof, table_row_t& tuple) 
{
    assert(_opened);
  
    _file->get_sorted(_index, &tuple);
    eof = (++_index > _file->_tuple_count);

    return (RCOK);
}


EXIT_NAMESPACE(shore);


#endif // __SHORE_SORT_BUF
