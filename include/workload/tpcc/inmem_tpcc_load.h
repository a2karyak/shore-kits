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


/////////////////////////////////////////////////////
// Class inmem_loader_impl


template <class InMemDS, class RowParser>
class inmem_loader_impl : public thread_t
{
private:
    c_str _fname; // file name    
    InMemDS* _ds; // data structure

    typedef typename RowParser::record_t record_t;

public:
    inmem_loader_impl(c_str aloadername, c_str afname, InMemDS* ads) : 
        thread_t(aloadername),
        _fname(afname),_ds(ads) 
    {
        TRACE( TRACE_DEBUG, "New (%s) (%s) (%p)\n", aloadername, _fname, _ds);
    }

    virtual ~inmem_loader_impl() { }

    void* run();

}; // EOF inmem_loader_impl


template <class InMemDS, class RowParser>
void* inmem_loader_impl<InMemDS, RowParser>::run() {


    TRACE( TRACE_DEBUG, " HALLO \n ");

    FILE* fd = fopen(_fname.data(), "r");

    if (fd == NULL) {

        //        THROW2( TrxException, "fopen() failed on (%s)\n", _fname.data());
        printf("fopen() failed on (%s)\n", _fname.data());

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

        _ds->insert(i, record.second);
        progress_update(&progress);
    }
    

    if ( fclose(fd) ) {

        printf("fclose() failed on (%s)\n", _fname.data());
        //THROW2( TrxException, "fclose() failed on (%s)\n", _fname.data());

        return (NULL);
    }    

    return (NULL);
}


EXIT_NAMESPACE(tpcc);

#endif
