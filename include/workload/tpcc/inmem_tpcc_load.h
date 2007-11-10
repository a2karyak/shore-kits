/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file inmem_tpcc_load.h
 *
 *  @brief Definition of the InMemory TPC-C loaders
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __INMEM_TPCC_LOAD_H
#define __INMEM_TPCC_LOAD_H

#include "util/progress.h"
#include "workload/tpcc/tpcc_tbl_parsers.h"

ENTER_NAMESPACE(tpcc);


///////////////////////////////////////////////////////////
// @class Abstract data_loader_t
//
// @brief Wrapper class that get a row parser and a
// filename. It opens the file and for each row it parses
// it calls the insert function
//
// @note The insert function is pure virtual. 


template <class RowParser>
class data_loader_t : public thread_t
{
private:
    c_str _fname; // file name    

    typedef typename RowParser::record_t record_t;

public:
    data_loader_t(c_str aloadername, c_str afname) : 
        thread_t(aloadername),
        _fname(afname) 
    {
    }

    virtual ~data_loader_t() { }

    virtual void* run();

    virtual int insert(int idx, record_t aRecord)=0; 

}; // EOF data_loader_t


template <class RowParser>
void* data_loader_t<RowParser>::run() {

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


template<class Table>
class save_table : public thread_t {
    Table &_table;
    c_str _fname;
public:
    save_table(Table &table, c_str fname)
	: thread_t(c_str("%s_save_thread", _table.get_name().data())),
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
};

template<class Table>
class restore_table : public thread_t {
    Table &_table;
    c_str _fname;
public:
    restore_table(Table &table, c_str fname)
	: thread_t(c_str("%s_restore_thread", _table.get_name().data())),
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
};

///////////////////////////////////////////////////////////
// @class inmem_array_loader_impl
//
// @brief Loader for InMemory Arrays

template <class InMemDS, class RowParser>
class inmem_array_loader_impl : public data_loader_t<RowParser>
{
private:
    InMemDS* _ds; // data structure

    typedef typename RowParser::record_t record_t;

public:

    inmem_array_loader_impl(c_str aloadername, c_str afname, InMemDS* ads) :
        data_loader_t<RowParser>(aloadername, afname),
        _ds(ads)
    {
        TRACE( TRACE_DEBUG, "Array Loader constructor\n");
    }

    virtual ~inmem_array_loader_impl() {
        TRACE( TRACE_DEBUG, "Array Loader destructor\n");
    }

    int insert(int idx, record_t aRecord) {
        _ds->insert(idx, aRecord.second);
        TRACE( TRACE_DEBUG, "%d\n", idx);
        return (0);
    }

}; // EOF inmem_array_loader_impl




///////////////////////////////////////////////////////////
// @class inmem_bptree_loader_impl
//
// @brief Loader for InMemory BPTrees

template <class InMemDS, class RowParser>
class inmem_bptree_loader_impl : public data_loader_t<RowParser>
{
    InMemDS* _ds; // data structure

    typedef typename RowParser::record_t record_t;

public:

    inmem_bptree_loader_impl(c_str aloadername, c_str afname, InMemDS* ads) :
        data_loader_t<RowParser>(aloadername, afname),
        _ds(ads)
    {
        TRACE( TRACE_DEBUG, "BPTree Loader constructor\n");
    }

    virtual ~inmem_bptree_loader_impl() {
        TRACE( TRACE_DEBUG, "BPTree Loader destructor\n");
    }

    int insert(int idx, record_t aRecord) {
        _ds->insert(aRecord.first, aRecord.second);
        if ((idx % 1000) == 0)
            TRACE( TRACE_DEBUG, "%d\n", idx);
        return (0);
    }

}; // EOF inmem_bptree_loader_impl



EXIT_NAMESPACE(tpcc);

#endif
