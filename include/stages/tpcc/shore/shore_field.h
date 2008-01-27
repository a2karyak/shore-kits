/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_field.h
 *
 *  @brief:  Description and current value of a field
 *
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

#include "stages/tpcc/shore/shore_file_desc.h"
#include "util.h"



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


/* Currently supported sql types. */
enum  sqltype_t {
    SQL_SMALLINT,   /* SMALLINT */
    SQL_INT,        /* INTEGER */
    SQL_FLOAT,      /* FLOAT */
    SQL_VARCHAR,    /* VARCHAR */
    SQL_CHAR,       /* CHAR */
    SQL_TIME,       /* TIMESTAMP */
    SQL_NUMERIC,    /* NUMERIC */
    SQL_SNUMERIC    /* SIGNED NUMERIC */
};



/*--------------------------------------------------------------------
 * @class field_desc_t
 *
 * @brief description and current value of the field.
 *--------------------------------------------------------------------*/

class field_desc_t {
private:
    /* description of the field */
    char        _name[MAX_FIELDNAME_LEN];   /* field name */

    sqltype_t   _type;                      /* type */
    short       _size;                      /* max_size */
    bool        _allow_null;                /* allow null? */

    char*       _keydesc;                   /* buffer for key description */

    /* current value of the field */
    union field_value_t {
	short        _smallint;     /* SMALLINT */
	int          _int;          /* INT */
	double       _float;        /* FLOAT */
	timestamp_t* _time;         /* TIME or DATE */
	char*        _string;       /* CHAR, VARCHAR, NUMERIC */
    }   _value;                     /* holding the value */

    bool     _null_flag;      /* null value? */

    char*    _data;         /* buffer for _value._time or _value._string */
    short    _data_size;    /* size of the buffer */

    int      _real_size;    /* real size of the value */

    /* allocate the space for _data */
    void     alloc_space(const int size);

public:
    // Constructor
    field_desc_t()
	: _type(SQL_SMALLINT), _allow_null(false), _size(0),
	  _keydesc(NULL), _null_flag(true), _data(NULL), _data_size(0),
	  _real_size(0)
    { 
        _name[0] = '\0'; 
    }
    
    ~field_desc_t() { 
        if (_data) 
            free(_data); 
        delete _keydesc; 
    }

    // Access Methods
    const char * name() const { return _name; }

    static bool is_variable_length(sqltype_t type) { 
        return (type == SQL_VARCHAR); 
    }

    bool        is_variable_length() const { 
        return is_variable_length(_type); 
    }

    int         maxsize() const { return _size; }
    int         realsize() const { 
        assert(_type == SQL_VARCHAR); 
        return _real_size; 
    }

    sqltype_t   type() const { return _type; }
    bool        allow_null() const { return _allow_null; }

    /* return key description for index creation */
    const char* keydesc();

    /* set the type description for the field. */
    void   setup(sqltype_t type,
                 const char * name,
                 short size = 0,
                 bool allow_null = false);


    ////////////////////////////////////
    // value related functions

    bool   is_null() const { return _null_flag; }

    bool   copy_value(void* data) const; /* copy current value out */


    /* set current value */
    void   set_value(const void * data, int length);
    void   set_int_value(const int data);
    void   set_smallint_value(const short data);
    void   set_float_value(const double data);
    void   set_time_value(const timestamp_t & data);
    void   set_string_value(const char * string, int len);
    void   set_var_string_value(const char * string, int len);

    void   set_min_value();
    void   set_max_value();
    
    bool   set_null() { _null_flag = true; return true; }


    /* get current value */
    int          get_int_value() const;
    short        get_smallint_value() const;
    void         get_string_value(char* string, const int bufsize) const;
    double       get_float_value() const;
    timestamp_t& get_time_value() const;

    bool    load_value_from_file(ifstream & is, const char delim);

    void    print_desc(ostream & os = cout);
    void    print_value(ostream & os = cout);

}; // EOF: field_desc_t


/** 
 *  class field_desc_t functions
 */

inline void field_desc_t::setup(sqltype_t type,
                                const char* name,
                                short size,
                                bool allow_null)
{
    memset(_name, 0, MAX_FIELDNAME_LEN);
    strncpy(_name, name, MAX_FIELDNAME_LEN);
    _type = type;

    switch (_type) {
    case SQL_SMALLINT:
        _real_size = _size = sizeof(short);
        break;

    case SQL_INT:
        _real_size = _size = sizeof(int);
        break;

    case SQL_FLOAT:
        _real_size = _size = sizeof(double);
        break;
    
    case SQL_TIME:
        _real_size = _data_size = _size = sizeof(timestamp_t);
        _data = (char *) malloc( sizeof( timestamp_t ) );
        _value._time = (timestamp_t *) _data;
        break;
    
    case SQL_VARCHAR:
        _size = size;
        break;

    case SQL_CHAR:
    case SQL_NUMERIC:
    case SQL_SNUMERIC:
        _real_size = _data_size = _size = size;
        _data = (char*)malloc( size );
        _value._string = _data;
        break;
    }
    _allow_null = allow_null;
}


inline const char* field_desc_t::keydesc()
{
    if (!_data) _data = (char*)malloc( MAX_KEYDESC_LEN );
  
    switch (_type) {
    case SQL_SMALLINT:  sprintf(_data, "i%d", _size); break;
    case SQL_INT:       sprintf(_data, "i%d", _size); break;
    case SQL_FLOAT:     sprintf(_data, "f%d", _size); break;
    case SQL_TIME:      sprintf(_data, "b%d", _size); break;
    case SQL_VARCHAR:   sprintf(_data, "b*%d", _size); break;
    case SQL_CHAR:
    case SQL_NUMERIC:
    case SQL_SNUMERIC:
        sprintf(_data, "b%d", _size);  break;
    }
    return _data;
}


inline void field_desc_t::alloc_space(const int len)
{
    assert(_type == SQL_VARCHAR);
    if (_data_size >= len) return;

    if (_data_size) { 
	if ( _data ) free( _data );
    }
    _data = (char*)malloc( len );
    _data_size = len;

    memset(_data, ' ', _data_size);

    _value._string = _data;
}


inline void field_desc_t::set_value(const void* data,
                                    int length)
{
    _null_flag = false;

    switch (_type) {
    case SQL_SMALLINT:
    case SQL_INT:
    case SQL_FLOAT:
	memcpy(&_value, data, _size); break;
    case SQL_TIME:
	memcpy(_value._time, data, _size); break;
    case SQL_VARCHAR:
	set_var_string_value((const char*) data, length); break;
    case SQL_CHAR:
    case SQL_NUMERIC:
    case SQL_SNUMERIC:
	_real_size = length;
	assert( _real_size <= _size );
	memset(_data, ' ', _data_size);
	memcpy(_value._string, data, length); break;
    }
}


inline void field_desc_t::set_min_value()
{
    _null_flag = false;

    switch (_type) {
    case SQL_SMALLINT:
	_value._smallint = MIN_SMALLINT;
	break;
    case SQL_INT:
	_value._int = MIN_INT;
	break;
    case SQL_FLOAT:
	_value._float = MIN_FLOAT;
	break;
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


inline void field_desc_t::set_max_value()
{
    _null_flag = false;

    switch (_type) {
    case SQL_SMALLINT:
	_value._smallint = MAX_SMALLINT;
	break;
    case SQL_INT:
	_value._int = MAX_INT;
	break;
    case SQL_FLOAT:
	_value._float = MAX_FLOAT;
	break;
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


inline bool field_desc_t::copy_value(void* data) const
{
    assert(!_null_flag);

    switch (_type) {
    case SQL_SMALLINT:
    case SQL_INT:
    case SQL_FLOAT:
        memcpy(data, &_value, _size);
        break;
    case SQL_TIME:
        memcpy(data, _value._time, _size);
        break;
    case SQL_VARCHAR:
        memcpy(data, _value._string, _real_size);
        break;
    case SQL_CHAR:
    case SQL_NUMERIC:
    case SQL_SNUMERIC:
        memset(data, ' ', _size);
        memcpy(data, _value._string, _real_size);
        break;
    }

    return (true);
}


inline bool field_desc_t::load_value_from_file(ifstream & is,
                                               const char delim)
{
    char* string;
    string = new char [10 * _size];
    is.get(string, 10*_size, delim);
    if (strlen(string) == 0) {
        delete [] string;
        return false;
    }

    if (strcmp(string, "(null)") == 0) {
        assert(_allow_null);
        _null_flag = true;
        delete [] string;
        return true;
    }

    _null_flag = false;

    switch (_type) {
    case SQL_SMALLINT:  _value._smallint = atoi(string); break;
    case SQL_INT:       _value._int = atoi(string); break;
    case SQL_FLOAT:     _value._float = atof(string); break;
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



// ---------------------------------
// ---- Setting value functions ----


inline void field_desc_t::set_int_value(const int data)
{
    assert(_type == SQL_INT || _type == SQL_SMALLINT);
    _null_flag = false;
    if (_type == SQL_SMALLINT)
        _value._smallint = data;
    else 
        _value._int = data;
}


inline void field_desc_t::set_smallint_value(const short data)
{
    assert(_type == SQL_SMALLINT || _type == SQL_INT);
    _null_flag = false;
    if (_type == SQL_SMALLINT)
        _value._smallint = data;
    else _value._int = data;
}

inline void field_desc_t::set_float_value(const double data)
{ 
    assert(_type == SQL_FLOAT);
    _null_flag = false;
    _value._float = data;
}

inline void field_desc_t::set_time_value(const timestamp_t& data)
{
    assert(_type == SQL_TIME);
    _null_flag = false;
    memcpy(_value._time, &data, _size);
}

inline void field_desc_t::set_string_value(const char* string,
                                           const int len)
{
    assert(_type == SQL_CHAR || _type == SQL_NUMERIC || _type == SQL_SNUMERIC);
    _null_flag = false;
    memset(_value._string, ' ', _data_size);
    _real_size = MIN(len, _data_size);
    memcpy(_value._string, string, _real_size);
}

inline void field_desc_t::set_var_string_value(const char* string,
                                               const int len)
{
    assert(_type == SQL_VARCHAR);
    _null_flag = false;

    _real_size = MIN(len, _size);
    alloc_space(_real_size);

    memcpy(_value._string, string, _real_size);
}



// ---------------------------------
// ---- Getting value functions ----


inline int field_desc_t::get_int_value() const
{
    assert(_type == SQL_INT);
    return _value._int;
}

inline short field_desc_t::get_smallint_value() const
{
    assert(_type == SQL_SMALLINT);
    return _value._smallint;
}

inline void field_desc_t::get_string_value(char* buffer,
                                           const int bufsize) const
{
    assert(_type == SQL_CHAR || _type == SQL_VARCHAR
           || _type == SQL_NUMERIC || _type == SQL_SNUMERIC);
    memset(buffer, 0, bufsize);
    memcpy(buffer, _value._string, MIN(bufsize, _real_size));
}

inline double field_desc_t::get_float_value() const
{
    assert(_type == SQL_FLOAT);
    return _value._float;
}

inline timestamp_t& field_desc_t::get_time_value() const
{
    assert(_type == SQL_TIME);
    return *(_value._time);
}




EXIT_NAMESPACE(shore);

#endif /* __SHORE_FIELD_H */
