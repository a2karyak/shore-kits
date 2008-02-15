/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_field.h
 *
 *  @brief:  Description and current value of a field (column)
 *
 *  @note:   field_desc_t  - the description of a field
 *           field_value_t - the value of a field
 *
 *  The description of the field includes type, size, and whether it
 *  allows null values. The value of the field is stored in a union.
 *  If the type is SQL_TIME or strings, the data is stored in _data and
 *  the union contains the pointer to it.  The space of _data is
 *  allocated at setup time for fixed length fields and at set_value
 *  time for variable length fields.
 *
 *  @author: Mengzhi Wang, April 2001
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#ifndef __SHORE_FIELD_H
#define __SHORE_FIELD_H

#include "util.h"

#include "sm/shore/shore_file_desc.h"




ENTER_NAMESPACE(shore);


/*--------------------------------------------------------------------
 * helper classes and functions
 *--------------------------------------------------------------------*/

/* Value for timestamp field */
class timestamp_t {
private:
    time_t   _time;

public:
    timestamp_t() { _time = time(NULL); }

    static   int size() { return sizeof(time_t); }
    char *   string() const {
	char *temp = ctime(&_time);
	temp[strlen(temp)-1] = '\0';
	return temp;
    }
};


int  compare_smallint(const void * d1, const void * d2);
int  compare_int(const void * d1, const void * d2);



/*---------------------------------------------------------------------
 * 
 * sql_type_t
 *
 * Currently supported sql types
 *
 *--------------------------------------------------------------------*/

enum  sqltype_t {
    SQL_SMALLINT,   /* SMALLINT */
    SQL_INT,        /* INTEGER */
    SQL_FLOAT,      /* FLOAT */
    //    SQL_DECIMAL,    /* DECIMAL - DOUBLE/FLOAT */
    SQL_VARCHAR,    /* VARCHAR */
    SQL_CHAR,       /* CHAR */
    SQL_TIME,       /* TIMESTAMP */
    SQL_NUMERIC,    /* NUMERIC */
    SQL_SNUMERIC    /* SIGNED NUMERIC */
};




/*---------------------------------------------------------------------
 *
 * @class: field_desc_t
 *
 * @brief: Description of a table field (column)
 *
 * @note:  This class is not thread safe. Thus, it is up to the caller
 *         to ensure thread safety. It is dangerous two threads to 
 *         modifyconcurrently the same field_desc_t object.
 *
 *--------------------------------------------------------------------*/

class field_desc_t {
private:
    char        _name[MAX_FIELDNAME_LEN];   /* field name */
    char*       _keydesc;                   /* buffer for key description */

    sqltype_t   _type;                      /* type */
    short       _size;                      /* max_size */
    bool        _allow_null;                /* allow null? */


public:

    /* --- construction --- */

    field_desc_t()
	: _keydesc(NULL), _type(SQL_SMALLINT), _size(0), _allow_null(true)
    { 
        _name[0] = '\0'; 
    }
    
    ~field_desc_t() { 
        if (_keydesc)
            delete _keydesc;
    }


    /* --- access methods --- */

    const char* name() const { return _name; }

    static bool is_variable_length(sqltype_t type) { 
        return (type == SQL_VARCHAR); 
    }

    inline bool        is_variable_length() const { 
        return is_variable_length(_type); 
    }

    inline int         maxsize() const { return _size; }
    inline sqltype_t   type() const { return _type; }
    inline bool        allow_null() const { return _allow_null; }

    /* return key description for index creation */
    const char* keydesc();

    /* set the type description for the field. */
    void   setup(const sqltype_t type,
                 const char* name,
                 const short size = 0,
                 const bool allow_null = false);

    /* for debugging */
    void    print_desc(ostream& os = cout);

}; // EOF: field_desc_t




/*--------------------------------------------------------------------
 * @struct field_value_t
 *
 * @brief Value of a table field
 *
 * @note This structure is not thread safe. Thus, it is up to the called
 *       to ensure thread safety. It is dangerous two threads to modify
 *       concurrently the same field_value_t object.
 *--------------------------------------------------------------------*/

struct field_value_t {

    /* --- member variables --- */

    field_desc_t*    _pfield_desc;   /* description of the field */
    bool             _is_setup;      /* flag if already setup */
    bool             _is_varlen;     /* flag if variable length, set at setup */
    bool             _null_flag;     /* null value? */

    /* value of a field */
    union s_field_value_t {
	short        _smallint;      /* SMALLINT */
	int          _int;           /* INT */
	double       _float;         /* FLOAT */
	timestamp_t* _time;          /* TIME or DATE */
	char*        _string;        /* CHAR, VARCHAR, NUMERIC */
    }   _value;                      /* holding the value */

    char*    _data;         /* buffer for _value._time or _value._string */
    int      _data_size;    /* size of the data buffer */
    int      _real_size;    /* real size of the value */


    /* --- construction --- */

    field_value_t() 
        :  _pfield_desc(NULL), _is_setup(false), _is_varlen(false), 
           _null_flag(true), _data(NULL), _data_size(0), _real_size(0)
    { 
    }


    field_value_t(field_desc_t* pfd) 
        : _pfield_desc(pfd), _is_setup(false), _is_varlen(false), 
          _null_flag(true), _data(NULL), _data_size(0), _real_size(0)
    { 
        setup(pfd); /* It will assert if pfd = NULL */
    }


    ~field_value_t() {
        if (_data)
            delete _data;
    }


    /* setup according to the given field_dest_t */
    void   setup(field_desc_t* pfd);

    inline bool          is_setup() { return (_is_setup); }
    inline field_desc_t* field_desc() { return (_pfield_desc); }


    /* return realsize of value */
    int    realsize() const { 
        return (_real_size);
    }


    /* ------------------------------- */
    /* --- value related functions --- */
    /* ------------------------------- */

   
    /* allocate the space for _data */
    void   alloc_space(const int size);


    /* set min/max allowed value */
    void   set_min_value();
    void   set_max_value();
    

    /* null field */
    inline bool   is_null() const { return _null_flag; }
    inline bool   set_null() { assert (_pfield_desc->allow_null()); _null_flag = true; return true; }

    /* var length */
    inline bool   is_variable_length() { assert (_is_setup); return (_is_varlen); }


    /* copy current value out */
    bool   copy_value(void* data) const;


    /* set current value */
    void   set_value(const void* data, const int length);
    void   set_int_value(const int data);
    void   set_smallint_value(const short data);
    void   set_float_value(const double data);
    void   set_decimal_value(const decimal data);
    void   set_time_value(const time_t data);    
    void   set_tstamp_value(const timestamp_t& data);
    void   set_string_value(const char* string, const int len);
    void   set_var_string_value(const char* string, const int len);


    /* get current value */
    int          get_int_value() const;
    short        get_smallint_value() const;
    void         get_string_value(char* string, const int bufsize) const;
    double       get_float_value() const;
    decimal      get_decimal_value() const;
    time_t       get_time_value() const;
    timestamp_t& get_tstamp_value() const;

    bool    load_value_from_file(ifstream & is, const char delim);

    /* access field description */
    inline field_desc_t* get_field_desc() { return (_pfield_desc); }
    inline void set_field_desc(field_desc_t* fd) { assert (fd); _pfield_desc = fd; }

    /* debugging */
    void    print_value(ostream & os = cout);

}; // EOF: field_value_t




/*********************************************************************
 *
 *  class field_desc_t functions
 *
 *********************************************************************/


/** @fn setup
 *
 *  @brief Sqltype specific setup
 */

inline void field_desc_t::setup(sqltype_t type,
                                const char* name,
                                short size,
                                bool allow_null)
{
    // name of field
    memset(_name, 0, MAX_FIELDNAME_LEN);
    strncpy(_name, name, MAX_FIELDNAME_LEN);
    _type = type;

    // size of field
    switch (_type) {
    case SQL_SMALLINT:
        _size = sizeof(short);
        break;

    case SQL_INT:
        _size = sizeof(int);
        break;

    case SQL_FLOAT:
        _size = sizeof(double);
        break;

//     case SQL_DECIMAL:
//         _size = sizeof(decimal);
//         break;
    
    case SQL_TIME:
        _size = sizeof(timestamp_t);
        break;
    
    case SQL_VARCHAR:
        _size = size;
        break;

    case SQL_CHAR:
    case SQL_NUMERIC:
    case SQL_SNUMERIC:
        _size = size;
        break;
    }

    // set if this field can contain null values
    _allow_null = allow_null;
}



inline const char* field_desc_t::keydesc()
{
    if (!_keydesc) _keydesc = (char*)malloc( MAX_KEYDESC_LEN );
  
    switch (_type) {
    case SQL_SMALLINT:  sprintf(_keydesc, "i%d", _size); break;
    case SQL_INT:       sprintf(_keydesc, "i%d", _size); break;
    case SQL_FLOAT:     sprintf(_keydesc, "f%d", _size); break;
        //    case SQL_DECIMAL:   sprintf(_keydesc, "f%d", _size); break;
    case SQL_TIME:      sprintf(_keydesc, "b%d", _size); break;
    case SQL_VARCHAR:   sprintf(_keydesc, "b*%d", _size); break;
    case SQL_CHAR:
    case SQL_NUMERIC:
    case SQL_SNUMERIC:
        sprintf(_keydesc, "b%d", _size);  break;
    }
    return (_keydesc);
}




/*********************************************************************
 *
 *  class field_value_t functions
 *
 *********************************************************************/


/** @fn setup
 *
 *  @brief Field specific setup for the value
 */

inline void field_value_t::setup(field_desc_t* pfd)
{
    assert (pfd);

    // if it is already setup for this field do nothing
    if (_is_setup && _pfield_desc == pfd)
        return;

    _pfield_desc = pfd;

    switch (_pfield_desc->type()) {
    case SQL_SMALLINT:
        _real_size = sizeof(short);
        break;

    case SQL_INT:
        _real_size = sizeof(int);
        break;

    case SQL_FLOAT:
        _real_size = sizeof(double);
        break;

//     case SQL_DECIMAL:
//         _real_size = sizeof(decimal);
//         break;
    
    case SQL_TIME:
        _real_size = _data_size = sizeof(timestamp_t);
        _data = (char*)malloc(sizeof(timestamp_t));
        _value._time = (timestamp_t*) _data;
        break;
    
    case SQL_VARCHAR:
        /** real_size is not set. it will be set by the set_value() function */
        _is_varlen = false;
        break;

    case SQL_CHAR:
    case SQL_NUMERIC:
    case SQL_SNUMERIC:
        _real_size = _data_size = _pfield_desc->maxsize();
        _data = (char*)malloc(_data_size);
        _value._string = _data;
        break;
    }    

    _is_setup = true;
}



/** @fn: alloc_space
 * 
 *  @brief: Allocates the requested space (param len). If it has already
 *          allocated enough returns immediately.
 */

inline void field_value_t::alloc_space(const int len)
{
    assert(_pfield_desc);
    assert(_pfield_desc->type() == SQL_VARCHAR);

    // check if already enough space
    if (_data_size >= len) return;

    // if not, release previously allocated space and allocate new
    if (_data_size) { 
	if (_data) free(_data);
    }
    _data = (char*)malloc(len);
    _data_size = len;

    // clear the contents of the allocated buffer
    memset(_data, ' ', _data_size);

    // the string value points to the allocated buffer
    _value._string = _data;
}


inline void field_value_t::set_value(const void* data,
                                     const int length)
{
    assert (_pfield_desc);

    _null_flag = false;

    switch (_pfield_desc->type()) {
    case SQL_SMALLINT:
    case SQL_INT:
    case SQL_FLOAT:
	memcpy(&_value, data, _pfield_desc->maxsize()); 
        break;
//     case SQL_DECIMAL:
// 	memcpy(&_value, data, _pfield_desc->maxsize()); 
//         break;
    case SQL_TIME:
	memcpy(_value._time, data, _pfield_desc->maxsize()); 
        break;
    case SQL_VARCHAR:
	set_var_string_value((const char*)data, length); 
        break;
    case SQL_CHAR:
    case SQL_NUMERIC:
    case SQL_SNUMERIC:
	_real_size = length;
	assert(_real_size <= _pfield_desc->maxsize());
	memset(_data, ' ', _data_size);
	memcpy(_value._string, data, length); break;
    }
}




/* --------------------------------- */
/* ---- Setting value functions ---- */
/* --------------------------------- */


inline void field_value_t::set_min_value()
{
    assert (_pfield_desc);

    _null_flag = false;

    switch (_pfield_desc->type()) {
    case SQL_SMALLINT:
	_value._smallint = MIN_SMALLINT;
	break;
    case SQL_INT:
	_value._int = MIN_INT;
	break;
    case SQL_FLOAT:
	_value._float = MIN_FLOAT;
	break;
//     case SQL_DECIMAL:
// 	_value._decimal = MIN_FLOAT;
// 	break;
    case SQL_VARCHAR:
    case SQL_CHAR:
    case SQL_NUMERIC:
    case SQL_SNUMERIC:
	_data[0] = '\0';
	_value._string = _data;
	break;
    case SQL_TIME:
	break;
    }
}


inline void field_value_t::set_max_value()
{
    assert (_pfield_desc);

    _null_flag = false;

    switch (_pfield_desc->type()) {
    case SQL_SMALLINT:
	_value._smallint = MAX_SMALLINT;
	break;
    case SQL_INT:
	_value._int = MAX_INT;
	break;
    case SQL_FLOAT:
	_value._float = MAX_FLOAT;
	break;
//     case SQL_DECIMAL:
// 	_value._decimal = MAX_FLOAT;
// 	break;
    case SQL_VARCHAR:
    case SQL_CHAR:
	memset(_data, 'z', _data_size);
	_value._string = _data;
	break;
    case SQL_NUMERIC:
    case SQL_SNUMERIC:
	memset(_data, '9', _data_size);
	_value._string = _data;
	break;
    case SQL_TIME:
	break;
    }
}



inline bool field_value_t::copy_value(void* data) const
{
    assert (_pfield_desc);
    assert (!_null_flag);
    assert (_real_size);

    switch (_pfield_desc->type()) {
    case SQL_SMALLINT:
    case SQL_INT:
    case SQL_FLOAT:
        memcpy(data, &_value, _real_size);
        break;
//     case SQL_DECIMAL:
//         memcpy(data, &_value, _real_size);
//         break;
    case SQL_TIME:
        memcpy(data, _value._time, _real_size);
        break;
    case SQL_VARCHAR:
        memcpy(data, _value._string, _real_size);
        break;
    case SQL_CHAR:
    case SQL_NUMERIC:
    case SQL_SNUMERIC:
        memset(data, ' ', _real_size);
        memcpy(data, _value._string, _real_size);
        break;
    }

    return (true);
}


inline bool field_value_t::load_value_from_file(ifstream & is,
                                                const char delim)
{
    assert (_pfield_desc);

    char* string;
    string = new char [10 * _pfield_desc->maxsize()];
    is.get(string, 10*_pfield_desc->maxsize(), delim);
    if (strlen(string) == 0) {
        delete [] string;
        return false;
    }

    if (strcmp(string, "(null)") == 0) {
        assert(_pfield_desc->allow_null());
        _null_flag = true;
        delete [] string;
        return true;
    }

    _null_flag = false;

    switch (_pfield_desc->type()) {
    case SQL_SMALLINT:  _value._smallint = atoi(string); break;
    case SQL_INT:       _value._int = atoi(string); break;
    case SQL_FLOAT:     _value._float = atof(string); break;
        //    case SQL_DECIMAL:   _value._decimal = decimal(atof(string)); break;
    case SQL_TIME:      break;
    case SQL_VARCHAR:   {
        if (string[0] == '\"') string[strlen(string)-1] = '\0';
        set_var_string_value(string+1, strlen(string)-1);
        break;
    }
    case SQL_CHAR:  {
        if (string[0] == '\"') string[strlen(string)-1] = '\0';
        set_string_value(string+1, strlen(string)-1);
        break;
    } 
    case SQL_NUMERIC:
    case SQL_SNUMERIC:
        set_string_value(string, strlen(string));
        break;
    }
    delete [] string;
    return true;
}



inline void field_value_t::set_int_value(const int data)
{
    assert (_pfield_desc);
    register sqltype_t t = _pfield_desc->type();
    assert (t == SQL_INT || t == SQL_SMALLINT);
    _null_flag = false;
    if (t == SQL_SMALLINT)
        _value._smallint = data;
    else 
        _value._int = data;
}


inline void field_value_t::set_smallint_value(const short data)
{
    assert (_pfield_desc);
    register sqltype_t t = _pfield_desc->type();
    assert (t == SQL_SMALLINT || t == SQL_INT);
    _null_flag = false;
    if (t == SQL_SMALLINT)
        _value._smallint = data;
    else _value._int = data;
}

inline void field_value_t::set_float_value(const double data)
{ 
    assert (_pfield_desc);
    assert (_pfield_desc->type() == SQL_FLOAT);
    _null_flag = false;
    _value._float = data;
}

inline void field_value_t::set_decimal_value(const decimal data)
{ 
    assert (_pfield_desc);
    assert (_pfield_desc->type() == SQL_FLOAT);
    _null_flag = false;
    _value._float = data.to_double();
}

inline void field_value_t::set_time_value(const time_t data)
{ 
    assert (_pfield_desc);
    assert (_pfield_desc->type() == SQL_FLOAT);
    _null_flag = false;
    _value._float = data;
}

// inline void field_value_t::set_decimal_value(const decimal data)
// { 
//     assert (_pfield_desc);
//     assert (_pfield_desc->type() == SQL_DECIMAL);
//     _null_flag = false;
//     _value._decimal = data;
// }

inline void field_value_t::set_tstamp_value(const timestamp_t& data)
{
    assert (_pfield_desc);
    assert (_pfield_desc->type() == SQL_TIME);
    _null_flag = false;
    memcpy(_value._time, &data, _real_size);
}

inline void field_value_t::set_string_value(const char* string,
                                            const int len)
{
    assert (_pfield_desc);
    assert (_pfield_desc->type() == SQL_CHAR || _pfield_desc->type() == SQL_NUMERIC ||
            _pfield_desc->type() == SQL_SNUMERIC);
    _null_flag = false;
    memset(_value._string, ' ', _data_size);
    _real_size = MIN(len, _data_size);
    memcpy(_value._string, string, _real_size);
}



/** @fn: set_var_string_value
 *
 *  @brief: Copies the string to the data buffer and sets real_size
 *
 *  @note: Only len chars are copied. If len > field->maxsize() then only
 *         maxsize() chars are copied.
 */

inline void field_value_t::set_var_string_value(const char* string,
                                                const int len)
{
    assert (_pfield_desc);
    assert (_pfield_desc->type() == SQL_VARCHAR);
    _null_flag = false;
    _real_size = MIN(len, _pfield_desc->maxsize());
    alloc_space(_real_size);
    memcpy(_value._string, string, _real_size);
}



/* --------------------------------- */
/* ---- Getting value functions ---- */
/* --------------------------------- */


inline int field_value_t::get_int_value() const
{
    assert (_pfield_desc);
    assert (_pfield_desc->type() == SQL_INT);
    return (_value._int);
}

inline short field_value_t::get_smallint_value() const
{
    assert (_pfield_desc);
    assert (_pfield_desc->type() == SQL_SMALLINT);
    return (_value._smallint);
}

inline void field_value_t::get_string_value(char* buffer,
                                           const int bufsize) const
{
    assert (_pfield_desc);
    assert (_pfield_desc->type() == SQL_CHAR || _pfield_desc->type() == SQL_VARCHAR ||
            _pfield_desc->type() == SQL_NUMERIC || _pfield_desc->type() == SQL_SNUMERIC);
    memset(buffer, 0, bufsize);
    memcpy(buffer, _value._string, MIN(bufsize, _real_size));
}

inline double field_value_t::get_float_value() const
{
    assert (_pfield_desc);
    assert (_pfield_desc->type() == SQL_FLOAT);
    return (_value._float);
}


inline decimal field_value_t::get_decimal_value() const
{
    assert (_pfield_desc);
    assert (_pfield_desc->type() == SQL_FLOAT);
    return (decimal(_value._float));
}

inline time_t field_value_t::get_time_value() const
{
    assert (_pfield_desc);
    assert (_pfield_desc->type() == SQL_FLOAT);
    return (_value._float);
}

// inline double field_value_t::get_decimal_value() const
// {
//     assert (_pfield_desc);
//     assert (_pfield_desc->type() == SQL_DECIMAL);
//     return (_value._decimal);
// }

inline timestamp_t& field_value_t::get_tstamp_value() const
{
    assert (_pfield_desc);
    assert (_pfield_desc->type() == SQL_TIME);
    return *(_value._time);
}




EXIT_NAMESPACE(shore);

#endif /* __SHORE_FIELD_H */
