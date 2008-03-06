/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_sort_buf.h
 *
 *  @brief:  In-memory sort buffer structure
 *
 *  @author: Ippokratis Pandis, January 2008
 */

#ifndef   __SHORE_SORT_BUF_H
#define   __SHORE_SORT_BUF_H

#include "sm/shore/shore_table_man.h"


ENTER_NAMESPACE(shore);


const int MIN_TUPLES_FOR_SORT = 100;



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


int compare_smallint(const void* d1, const void* d2)
{
    short data1 = *((short*)d1);
    short data2 = *((short*)d2);
    if (data1 > data2) return (1);
    if (data1 == data2) return (0);
    return (-1);
}

int compare_int(const void* d1, const void* d2)
{
    int data1 = *((int*)d1);
    int data2 = *((int*)d2);
    if (data1 > data2) return (1);
    if (data1 == data2) return (0);
    return (-1);
}

template <typename T>
int compare(const void* d1, const void* d2)
{
    T data1 = *((T*)d1);
    T data2 = *((T*)d2);
    if (data1 > data2) return (1);
    if (data1 == data2) return (0);
    return (-1);
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
 *        Supported sql_types_t: SQL_INT, SQL_SMALLINT.
 *
 **********************************************************************/

class sort_iter_impl;


/**********************************************************************
 *
 * @class:   sort_buffer_t
 *
 * @brief:   Description of a sort buffer
 *
 **********************************************************************/

class sort_buffer_t : public table_desc_t 
{
public:

    sort_buffer_t(int field_count)
        : table_desc_t("SORT_BUF", field_count)
    { 
    }

    ~sort_buffer_t() 
    { 
    }

    /* set the schema - accepts only fixed length */
    void setup(const int index, sqltype_t type, const int len = 0) 
    {
        assert((index>=0) && (index<_field_count));
        _desc[index].setup(type, "", len);
        assert(!_desc[index].is_variable_length());
        assert(!_desc[index].allow_null());
    }

    /* needed by table_desc_t in order not to be abstract */
    bool read_tuple_from_line(table_row_t& , char* ) {
        assert (false); // should not be called'
        return (false);
    }

}; // EOF: sort_buffer_t



/**********************************************************************
 *
 * @class:   sort_man_impl
 *
 * @warning: NO THREAD-SAFE the caller should make sure that only one
 *           thread is accessing objects of this class
 *
 **********************************************************************/

class sort_man_impl : public table_man_impl<sort_buffer_t>
{    
    typedef row_impl<sort_buffer_t> sorter_tuple;    
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
    bool get_sorted(const int index, sorter_tuple* ptuple); 

public:

    sort_man_impl(sort_buffer_t* aSortBufferDesc, rep_row_t* aprow)
        : table_man_impl<sort_buffer_t>(aSortBufferDesc),
          _sort_buf(NULL), _tuple_size(0), _tuple_count(0), _buf_size(0), 
          _preprow(aprow), _is_sorted(false)
    {
    }

    ~sort_man_impl()
    {
        if (_sort_buf)
            delete [] _sort_buf;
    }


    /* add current tuple to the sort buffer */
    void add_tuple(sorter_tuple& atuple);

    /* return a sort iterator */
    w_rc_t get_sort_iter(ss_m* db, sort_iter_impl* &sort_iter);

    /* sort tuples on the first field value */
    void   sort();

    inline int count() { return (_tuple_count); }

    void   reset();

}; // EOF: sort_man_impl


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

typedef tuple_iter_t<sort_buffer_t, int, row_impl<sort_buffer_t> > sort_scan_t;

class sort_iter_impl : public sort_scan_t 
{
    typedef row_impl<sort_buffer_t> table_tuple;

private:

    sort_man_impl* _manager;
    int            _index;

public:

    sort_iter_impl(ss_m* db, sort_buffer_t* psortbuf, sort_man_impl* psortman)
        : tuple_iter_t(db, psortbuf), _manager(psortman), _index(0)
    { 
        assert (_manager);
        assert (_file);
        W_COERCE(open_scan());
    }


    /* ------------------------------ */
    /* --- sorted iter operations --- */
    /* ------------------------------ */
    
    w_rc_t open_scan();
    w_rc_t close_scan() { return (RCOK); };
    w_rc_t next(ss_m* db, bool& eof, table_tuple& tuple);

    void   reset();

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
 *  @brief: Calculate the tuple size and allocate the initial memory
 *          buffer for the tuples.
 *
 *********************************************************************/

inline void sort_man_impl::init()
{
    assert (_ptable);

    /* calculate tuple size */
    _tuple_size = 0;
    for (int i=0; i<_tuple_count; i++)
             _tuple_size += _ptable->desc(i)->fieldmaxsize();

    /* allocate size for MIN_TUPLES_FOR_SORT tuples */
    _sort_buf = new char[MIN_TUPLES_FOR_SORT*_tuple_size]; 
    _buf_size = MIN_TUPLES_FOR_SORT;

    _is_sorted = false;
}



/********************************************************************* 
 *
 *  @fn:    reset
 *  
 *  @brief: Clear the buffer and wait for new tuples
 *
 *********************************************************************/

inline void sort_man_impl::reset()
{
    assert (_ptable);
    // the soft_buf should be set
    assert (_sort_buf);
    // if buf_size>0 means that the manager has already been set
    assert (_buf_size); 
    // no need to calculate tuple size
    assert (_tuple_size);

    // memset the buffer
    memset(_sort_buf, 0, _buf_size);
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

inline void sort_man_impl::add_tuple(sorter_tuple& atuple)
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
    format(&atuple, *_preprow);
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

inline void sort_man_impl::sort()
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
    switch (_ptable->desc(0)->type()) {
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

inline w_rc_t sort_man_impl::get_sort_iter(ss_m* db,
                                           sort_iter_impl* &sort_iter)
{
    sort_iter = new sort_iter_impl(db, _ptable, this);
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

inline bool sort_man_impl::get_sorted(const int index, sorter_tuple* ptuple)
{
    CRITICAL_SECTION(cs, _sorted_lock);

    if (_is_sorted) {
        if (index >=0 && index < _tuple_count) {
            return (load(ptuple, _sort_buf + (index*_tuple_size)));
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
    assert (_file->field_count()>0);

    _manager->sort();

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

inline w_rc_t sort_iter_impl::next(ss_m* db, bool& eof, table_tuple& tuple) 
{
    assert(_opened);
  
    _manager->get_sorted(_index, &tuple);
    eof = (++_index > _manager->_tuple_count);
    return (RCOK);
}


/********************************************************************* 
 *
 *  @fn:    reset
 *  
 *  @brief: Clear the fields and prepares for re-use
 *
 *********************************************************************/

inline void sort_iter_impl::reset() 
{
    // the sorter_manager should already be set
    assert (_manager);
    _index=0;
}


EXIT_NAMESPACE(shore);


#endif // __SHORE_SORT_BUF
