/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_field.cpp
 *
 *  @brief:  Implementation of the table field description
 *
 *  @author: Mengzhi Wang, April 2001
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#include "stages/tpcc/shore/shore_field.h"

using namespace shore;


/* ------------------------------- */
/* --- helper global functions --- */
/* ------------------------------- */

int  compare_smallint(const void * d1, const void * d2)
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

void  field_desc_t::print_value(ostream & os)
{
    if (_null_flag) {
	os << "(null)";
	return;
    }
    switch (_type) {
    case SQL_SMALLINT:
	os <<_value._smallint;
	break;
    case SQL_INT:
	os << _value._int;
	break;
    case SQL_FLOAT:
	os << _value._float;
	break;
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
