/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _TPCH_TUPLE_PARSERS_H
#define _TPCH_TUPLE_PARSERS_H


/* exported functions */

void tpch_tuple_parser_CUSTOMER(char* dest, char* linebuffer);
void tpch_tuple_parser_LINEITEM(char* dest, char* linebuffer);
void tpch_tuple_parser_NATION(char* dest, char* linebuffer);
void tpch_tuple_parser_ORDERS(char* dest, char* linebuffer);
void tpch_tuple_parser_PART(char* dest, char* linebuffer);
void tpch_tuple_parser_PARTSUPP(char* dest, char* linebuffer);
void tpch_tuple_parser_REGION(char* dest, char* linebuffer);
void tpch_tuple_parser_SUPPLIER(char* dest, char* linebuffer);


#endif
