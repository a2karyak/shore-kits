/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_loader.h
 *
 *  @brief Generic TPC-C loaders
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCC_DATA_LOADER_H
#define __TPCC_DATA_LOADER_H


#include "util/c_str.h"
#include "util/progress.h"
#include "util/c_str.h"
#include "workload/tpcc/tpcc_tbl_parsers.h"



ENTER_NAMESPACE(tpcc);


///////////////////////////////////////////////////////////
// @class Abstract data_loader_t
//
// @brief Wrapper class that gets a row parser, a filename, 
// and a thread implementation. It opens the file and for 
// each row it parses it calls the insert function
//
// @note The insert function is pure virtual. 

template <class RowParser, class ThreadImpl>
class data_loader_t : public ThreadImpl
{
private:
    c_str _fname; // file name    

    typedef typename RowParser::record_t record_t;

public:

    data_loader_t(c_str aloadername, c_str afname) : 
        ThreadImpl(aloadername),
        _fname(afname) 
    {
    }

    virtual ~data_loader_t() { }
    virtual void* run();
    virtual int insert(int idx, record_t aRecord)=0; 

}; // EOF data_loader_t


/** data_loader_t methods */

template <class RowParser, class ThreadImpl>
void* data_loader_t<RowParser, ThreadImpl>::run() {

    TRACE( TRACE_DEBUG, "Loading (%s)\n", _fname.data());

    FILE* fd = fopen(_fname.data(), "r");

    if (fd == NULL) {        
        THROW2( TrxException, "fopen() failed on (%s)\n", _fname.data());
        //printf("fopen() failed on (%s)\n", _fname.data());

        return (NULL);
    }

    // Insert rows one by one
    unsigned long progress = 0;
    char linebuffer[MAX_LINE_LENGTH];
    RowParser parser;
    record_t record;
    int i = 0;

    for(i=0; fgets(linebuffer, MAX_LINE_LENGTH, fd); i++) {
        record = parser.parse_row(linebuffer);

        insert(i, record);
        progress_update(&progress);
    }
    

    if ( fclose(fd) ) {
        //printf("fclose() failed on (%s)\n", _fname.data());
        THROW2( TrxException, "fclose() failed on (%s)\n", _fname.data());

        return (NULL);
    }    

    return (NULL);
}


///////////////////////////////////////////////////////////
// @class save_table
//
// @brief Wrapper class that saves a table

template<class Table, class ThreadImpl>
class save_table : public ThreadImpl {
    Table& _table;
    c_str  _fname;

public:
    save_table(Table &table, c_str fname)
	: ThreadImpl(c_str("%s_save_thread", table.get_name().data())),
	  _table(table), _fname(fname)
    {
    }

    virtual void* run() {
	TRACE( TRACE_DEBUG, "Saving %s table to (%s)\n", _table.get_name().data(), _fname.data());

	guard<FILE> fd = fopen(_fname.data(), "w");

	if (fd == NULL) {        
	    THROW2( TrxException, "fopen() failed on (%s)\n", _fname.data());
	    //printf("fopen() failed on (%s)\n", _fname.data());
	}

	try {
	    _table.save(fd);
	}
	catch(int error) {
	    THROW_IF(FileException, error);
	}
	
	return NULL;
    }

}; // EOF: save_table



///////////////////////////////////////////////////////////
// @class restore_table
//
// @brief Wrapper class that restores a table

template<class Table, class ThreadImpl>
class restore_table : public ThreadImpl {
    Table &_table;
    c_str _fname;

public:

    restore_table(Table &table, c_str fname)
	: ThreadImpl(c_str("%s_restore_thread", table.get_name().data())),
	  _table(table), _fname(fname)
    {
    }

    virtual void* run() {
	TRACE( TRACE_DEBUG, "Restoring table %s from  (%s)\n", _table.get_name().data(), _fname.data());

	guard<FILE> fd = fopen(_fname.data(), "r");

	if (fd == NULL) {        
	    THROW2( TrxException, "fopen() failed on (%s)\n", _fname.data());
	    //printf("fopen() failed on (%s)\n", _fname.data());
	}

	try {
	    _table.restore(fd);
	}
	catch(int error) {
	    THROW_IF(FileException, error);
	}
	
	return NULL;
    }

}; // EOF: restore_table



EXIT_NAMESPACE(tpcc);

#endif
