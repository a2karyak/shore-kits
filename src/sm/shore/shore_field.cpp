/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_field.cpp
 *
 *  @brief:  Implementation of the table field description and value
 *
 *  @author: Mengzhi Wang, April 2001
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#include "sm/shore/shore_field.h"

using namespace shore;



/* ----------------------------------- */
/* --- @class field_desc_t methods --- */
/* ------------------------------------*/


void  field_desc_t::print_desc(ostream & os)
{
    os << "Field " << _name << "\t";
    switch (_type) {
    case SQL_SMALLINT:
	os << "Type: SMALLINT \t size: " << sizeof(short) << endl;
	break;
    case SQL_INT:
	os << "Type: INT \t size: " << sizeof(int) << endl;
	break;
    case SQL_FLOAT:
	os << "Type: FLOAT \t size: " << sizeof(double) << endl;
	break;
//     case SQL_DECIMAL:
// 	os << "Type: DECIMAL \t size: " << sizeof(decimal) << endl;
// 	break;
    case SQL_TIME:
	os << "Type: TIMESTAMP \t size: " << timestamp_t::size() << endl;
	break;
    case  SQL_VARCHAR:
	os << "Type: VARCHAR \t size: " << _size << endl;
	break;
    case SQL_CHAR:
	os << "Type: CHAR \t size: " << _size << endl;
	break;
    case SQL_NUMERIC:
	os << "Type: NUMERIC \t size: " << _size << endl;
	break;
    case SQL_SNUMERIC:
	os << "Type: SNUMERIC \t size: " << _size << endl;
	break;
    }
} 



/* ------------------------------------ */
/* --- @class field_value_t methods --- */
/* -------------------------------------*/

void  field_value_t::print_value(ostream & os)
{
    assert (_pfield_desc);

    if (_null_flag) {
	os << "(null)";
	return;
    }

    switch (_pfield_desc->type()) {
    case SQL_SMALLINT:
	os <<_value._smallint;
	break;
    case SQL_INT:
	os << _value._int;
	break;
    case SQL_FLOAT:
	os << _value._float;
	break;
//     case SQL_DECIMAL:
// 	os << _value._decimal;
// 	break;
    case SQL_TIME:
	os << _value._time->string();
	break;
    case SQL_VARCHAR:
    case SQL_CHAR:
	os << "\"";
	for (int i=0; i<_real_size; i++) {
	    if (_value._string[i]) os << _value._string[i];
	}
	os << "\"";
	break;
    case SQL_NUMERIC:
    case SQL_SNUMERIC: {
	for (int i=0; i<_real_size; i++) {
	    if (_value._string[i]) os << _value._string[i];
	}
	break;
    }
    }
}



/*********************************************************************
 *
 *  @fn:    get_debug_str
 *
 *  @brief: Return a string with the value of the specific type and value. 
 *          Used for debugging purposes.
 *
 *********************************************************************/

void field_value_t::get_debug_str(char* &buf)
{
    assert (_pfield_desc);
    buf = new char[MAX_LINE_LENGTH];
    memset(buf, '\0', MAX_LINE_LENGTH);

    if (_null_flag) {
	sprintf(buf, "(null)");
    }

    switch (_pfield_desc->type()) {
    case SQL_SMALLINT:
        sprintf(buf, "SQL_SMALLINT: \t%d", _value._smallint);
	break;
    case SQL_INT:
        sprintf(buf, "SQL_INT:      \t%d", _value._int);
	break;
    case SQL_FLOAT:
        sprintf(buf, "SQL_FLOAT:    \t%.2f", _value._float);
	break;
    case SQL_TIME:
        sprintf(buf, "SQL_TIME:     \t%s", _value._time->string());
	break;
    case SQL_VARCHAR:
        strcat(buf, "SQL_VARCHAR:  \t");
        strncat(buf, _value._string, _real_size);
        break;
    case SQL_CHAR:
        strcat(buf, "SQL_CHAR:     \t");
        strncat(buf, _value._string, _real_size);
	break;
    case SQL_NUMERIC:
        strcat(buf, "SQL_NUMERIC:  \t");
        strncat(buf, _value._string, _real_size);
        break;
    case SQL_SNUMERIC: {
        strcat(buf, "SQL_sNUMERIC: \t");
        strncat(buf, _value._string, _real_size);
	break;
    }
    }
}
