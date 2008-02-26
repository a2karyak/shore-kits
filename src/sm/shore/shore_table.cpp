/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_table.cpp
 *
 *  @brief Implementation of shore_table class
 *
 *  @author: Mengzhi Wang, April 2001
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#include "sm/shore/shore_table.h"
#include "sm/shore/shore_helper_loader.h"

using namespace shore;



/* ----------------------------------- */
/* --- @class table_desc_t methods --- */
/* ----------------------------------- */


#define VAR_SLOT(start, offset)   ((offset_t*)((start)+(offset)))
#define SET_NULL_FLAG(start, offset)                            \
    (*(char*)((start)+((offset)>>3))) &= (1<<((offset)>>3))
#define IS_NULL_FLAG(start, offset)                     \
    (*(char*)((start)+((offset)>>3)))&(1<<((offset)>>3))




/* -------------------------- */
/* --- bulkload utilities --- */
/* -------------------------- */



/********************************************************************* 
 *
 *  @fn:    load_from_file
 *  
 *  @brief: Loads a table from a datafile which has name. "tablename".tbl
 *
 *  @note:  The index file should already been created by create_index()  
 *
 *********************************************************************/

w_rc_t table_desc_t::load_from_file(ss_m* db, const char* fname)
{
    /* 0. get filename */
    char filename[MAX_FILENAME_LEN];

    if (fname == NULL) {
        // if null use "name().tbl" file
        strcpy(filename, name());
        strcat(filename, ".tbl");
    }
    else {
        int sz = (strlen(fname)>MAX_FILENAME_LEN ? strlen(fname) : MAX_FILENAME_LEN);
        strncpy(filename, fname, sz);
    }

    FILE* fd = fopen(filename, "r");
    if (fd == NULL) {
        TRACE( TRACE_ALWAYS, "fopen() failed on (%s)\n", filename);
	return RC(se_NOT_FOUND);
    }    
    TRACE( TRACE_DEBUG, "Loading (%s) table from (%s) file...\n", 
           _name, filename);
    
    W_DO(db->begin_xct());
    
    /* 1. create the warehouse table */
    W_DO(create_table(db));
         
    /* 2. append the tuples */
    append_file_i file_append(_fid);    

    /* 3a. read the file line-by-line */
    /* 3b. convert each line to a tuple for this table */
    /* 3c. insert tuple to table */
    register int tuple_count = 0;
    register int mark = COMMIT_ACTION_COUNT;

    table_row_t  tuple(this);
    bool btread = false;
    char linebuffer[MAX_LINE_LENGTH];
    char* pdest = NULL;
    int   bufsz = 0;
    int tsz = 0;

    for(int i=0; fgets(linebuffer, MAX_LINE_LENGTH, fd); i++) {

        // read tuple and put it in a table_row_t
        btread = read_tuple_from_line(tuple, linebuffer);

        // (ip) for debugging
        //if (!strcmp(name(), "CUSTOMER")) tuple.print_tuple();

        // append it to the table
        tsz = tuple.format(pdest, bufsz);
        assert (pdest); // (ip) if NULL invalid
	W_DO(file_append.create_rec(vec_t(), 0,
				    vec_t(pdest, tsz),
				    tuple._rid));

	if(i >= mark) {
	    W_COERCE(db->commit_xct());
            TRACE( TRACE_DEBUG, "load(%s): %d\n", name(), tuple_count);
	    W_COERCE(db->begin_xct());
	    mark += COMMIT_ACTION_COUNT;
	}	    
        tuple_count++;
    }
    
    /* 3. commit and print statistics */
    W_DO(db->commit_xct());

    if (pdest)
        delete [] pdest;


    TRACE( TRACE_DEBUG, "(%s) # of records inserted: %d\n",
           _name, tuple_count);

    /* 4. update the index structures */
    return (bulkload_all_indexes(db));
}



/********************************************************************* 
 *
 *  @fn:    bulkload_index
 *  
 *  @brief: Iterates a file and inserts (one by one) the corresponding 
 *          entries to the specified index. 
 *
 *  @note:  The index file should have already been created by create_index()
 *
 *********************************************************************/

w_rc_t table_desc_t::bulkload_index(ss_m* db,
                                    index_desc_t* index)
{
    assert (index);

    TRACE( TRACE_DEBUG, "Building index: %s\n", index->name());
    
    W_DO(db->begin_xct());
    
    /* 1. open a (table) scan iterator over the table and create 
     * an index helper loader thread 
     */

    table_scan_iter_impl* iter;
    W_DO(get_iter_for_file_scan(db, iter));

    bool eof = false;
    table_row_t row(this);
    time_t tstart = time(NULL);
    int rowscanned = 0;

    // fire up the index loading helper
    guard<index_loading_smt_t> idxld = new 
        index_loading_smt_t(c_str("idxld"), db, this, index, &row);    

    CRITICAL_SECTION(cs, &idxld->_cs_mutex);
    cs.pause();

    idxld->fork();

    /* 2. iterate over the whole table and insert the corresponding 
     *    index entries, using the index loading helper thread 
     */
    while (!eof) {

        bool row_consumed = false;
        while (!row_consumed) {

            //*** START: CS ***//
            cs.resume();

            if (!idxld->_has_to_consume) {
                // if the consumer is waiting 
            
                //*** PRODUCE ***//
                W_DO(iter->next(db, eof, row));
                idxld->_has_to_consume = true;
                row_consumed = true;
                idxld->_start = true;

                if (eof)
                    idxld->_finish = true;
                else
                    rowscanned++;
            }
        
            cs.pause();
            //*** EOF: CS ***//
        }
    }
    delete iter;

    W_DO(db->commit_xct());

    // join the index loading helper and check value
    idxld->join();
    if (idxld->rv()) {
        TRACE( TRACE_ALWAYS, "Error in Index (%s) loading helper...\n",
               index->name());
        return (RC(se_ERROR_IN_IDX_LOAD));
    }

    /* 5. print stats */
    time_t tstop = time(NULL);
    int idxcount = idxld->count();
    TRACE( TRACE_TRX_FLOW, "Index (%s) loaded (%d) entries in (%d) secs...\n",
           index->name(), idxcount, (tstop - tstart));
    // make sure that the correct number of rows were inserted
    assert (rowscanned == idxcount); 

    cs.hand_off();

    return (RCOK);
}



/********************************************************************* 
 *
 *  @fn:    bulkload_index (by name)
 *  
 *  @brief: Finds the appropriate index given the passed name, and
 *          calls the bulkload_index function that does the actual loading.
 *
 *********************************************************************/

w_rc_t table_desc_t::bulkload_index(ss_m* db,
                                    const char* name)
{
    index_desc_t* index = (_indexes ? _indexes->find_by_name(name) : NULL);

    if (index)
        return bulkload_index(db, index);
    return RC(se_INDEX_NOT_FOUND);
}



/********************************************************************* 
 *
 *  @fn:    bulkload_all_indexes
 *  
 *  @brief: Iterates over all the indexes of the table, and
 *          calls the bulkload_index function that does the loading.
 *
 *********************************************************************/

w_rc_t table_desc_t::bulkload_all_indexes(ss_m* db)
{
    TRACE( TRACE_DEBUG, "Start building indices for (%s)\n", _name);
  
    index_desc_t* index = _indexes;

    while (index) {
	// build one index at each iteration.
        w_rc_t e = bulkload_index(db, index);
        if (e) {
            TRACE( TRACE_ALWAYS, "Index (%s) loading aborted [0x%x]\n",
                   index->name(), e.err_num());
            assert (false); // (ip) should never happen
        }
	index = index->next();
    }

    TRACE( TRACE_DEBUG, "End building indices for (%s)\n", _name);
    return RCOK;
}


/* --------------------------------- */
/* --- access data through index --- */
/* --------------------------------- */


/********************************************************************* 
 *
 *  @fn:    index_probe
 *  
 *  @brief: Finds the rid of the specified key using a certain index
 *
 *  @note:  The key is parsed from the tuple that it is passed as parameter
 *
 *********************************************************************/

w_rc_t  table_desc_t::index_probe(ss_m* db,
                                  index_desc_t* index,
                                  table_row_t* ptuple,
                                  lock_mode_t lmode)
{
    assert (index);
    assert (ptuple); 

    bool     found = false;
    smsize_t len = sizeof(rid_t);

    /* 1. ensure valid index */
    W_DO(index->check_fid(db));

    /* 2. find the tuple in the B+tree */

    char* pdest = NULL;
    int   bufsz = 0;

    int key_sz = format_key(index, ptuple, pdest, bufsz);
    assert (pdest); // (ip) if NULL invalid key

    W_DO(ss_m::find_assoc(index->fid(),
			  vec_t(pdest, key_sz),
			  &(ptuple->_rid),
			  len,
			  found));
    delete [] pdest;

    if (!found) return RC(se_TUPLE_NOT_FOUND);

    /* 3. read the tuple */
    pin_i pin;
    W_DO(pin.pin(ptuple->rid(), 0, lmode));
    if (!ptuple->load(pin.body())) return RC(se_WRONG_DISK_DATA);
    pin.unpin();

    //    ptuple->print_tuple();
  
    return RCOK;
}



/* ----------------------------------------- */
/* --- create physical table and indexes --- */
/* ----------------------------------------- */


/********************************************************************* 
 *
 *  @fn:    create_table
 *
 *  @brief: Creates the physical table and all the corresponding indexes
 *
 *********************************************************************/

w_rc_t table_desc_t::create_table(ss_m* db)
{
    if (!is_vid_valid() || !is_root_valid())
	W_DO(find_root_iid(db));

    /* create the table */
    W_DO(db->create_file(vid(), _fid, smlevel_3::t_regular));

    /* add table entry to the metadata tree */
    file_info_t file;
    file.set_ftype(FT_REGULAR);
    file.set_fid(_fid);
    W_DO(ss_m::create_assoc(root_iid(),
			    vec_t(name(), strlen(name())),
			    vec_t(&file, sizeof(file_info_t))));
    

    /* create all the indexes of the table */
    index_desc_t* index = _indexes;
    while (index) {
	stid_t iid;

        /* create index */
	W_DO(db->create_index(_vid,
                              (index->is_unique() ? ss_m::t_uni_btree : ss_m::t_btree),
                              ss_m::t_regular,
                              index_keydesc(index),
                              ss_m::t_cc_kvl,
                              iid));
	index->set_fid(iid);

        /* add index entry to the metadata tree */
        if (index->is_primary())
            file.set_ftype(FT_PRIMARY_IDX);
        else
            file.set_ftype(FT_IDX);

	file.set_fid(iid);
	W_DO(db->create_assoc(root_iid(),
                              vec_t(index->name(), strlen(index->name())),
                              vec_t(&file, sizeof(file_info_t))));
				
        /* move to next index */
	index = index->next();
    }
    
    return (RCOK);
}


/****************************************************************** 
 *  
 *  @fn:    create_index
 *
 *  @brief: Create a regular or primary index on the table
 *
 *  @note:  This only creates the index decription for the index in memory. 
 *          Call bulkload_index to create and populate the index on disks.
 *
 ******************************************************************/

bool table_desc_t::create_index(const char* name,
                                const int* fields,
                                const int num,
                                const bool unique,
                                const bool primary)
{
    index_desc_t* p_index = new index_desc_t(name, num, fields, unique);

    /* check the validity of the index */
    for (int i=0; i<num; i++)  {
        assert(fields[i] >= 0 && fields[i] < _field_count);

        /* only the last field in the index can be variable lengthed */
        if (_desc[fields[i]].is_variable_length() && i != num-1) {
            assert(false);
        }
    }

    /* link it to the list */
    if (_indexes == NULL) _indexes = p_index;
    else _indexes->insert(p_index);

    /* add as primary */
    if (p_index->is_unique() && p_index->is_primary())
        _primary_idx = p_index;

    return true;
}


bool  table_desc_t::create_primary_idx(const char* name,
                                       const int* fields,
                                       const int num)
{
    index_desc_t* p_index = new index_desc_t(name, num, fields, true, true);

    /* check the validity of the index */
    for (int i=0; i<num; i++) {
        assert(fields[i] >= 0 && fields[i] < _field_count);

        /* only the last field in the index can be variable lengthed */
        if (_desc[fields[i]].is_variable_length() && i != num-1) {
            assert(false);
        }
    }

    /* link it to the list of indexes */
    if (_indexes == NULL) _indexes = p_index;
    else _indexes->insert(p_index);

    /* make it the primary index */
    _primary_idx = p_index;

    return (true);
}



/* -------------------------- */
/* --- tuple manipulation --- */
/* -------------------------- */


/********************************************************************* 
 *
 *  @fn:    add_tuple
 *
 *  @brief: Inserts a tuple to a table and all the indexes of the table
 *
 *  @note:  This function should be called in the context of a trx.
 *          The passed tuple should be formed. If everything goes as
 *          expected the _rid of the tuple will be set. 
 *
 *********************************************************************/

w_rc_t table_desc_t::add_tuple(ss_m* db, table_row_t* tuple)
{
    assert (tuple);

    /* 1. find the file */
    W_DO(check_fid(db));

    /* 2. append the tuple */
    char* pdest = NULL;
    int   bufsz = 0;

    int tsz = tuple->format(pdest, bufsz);
    assert (pdest); // (ip) if NULL invalid

    W_DO(db->create_rec(fid(), vec_t(), tsz,
                        vec_t(pdest, tsz),
                        tuple->_rid));


    /* 3. update the indexes */
    index_desc_t* index = _indexes;
    int ksz = 0;

    while (index) {
        ksz = tuple->format_key(index, pdest, bufsz);
        assert (pdest); // (ip) if dest == NULL there is invalid key

        W_DO(index->find_fid(db));
	W_DO(db->create_assoc(index->fid(),
                              vec_t(pdest, ksz),
                              vec_t(&(tuple->_rid), sizeof(rid_t))));

        /* move to next index */
	index = index->next();
    }
    delete [] pdest;

    return (RCOK);
}



/********************************************************************* 
 *
 *  @fn:    update_tuple
 *
 *  @brief: Upldates a tuple from a table
 *
 *  @note:  This function should be called in the context of a trx.
 *          The passed tuple should be valid.
 *          There is no need of updating the indexes.
 *
 *********************************************************************/

w_rc_t table_desc_t::update_tuple(ss_m* db, table_row_t* tuple)
{
    assert (tuple);

    if (!tuple->is_rid_valid()) return RC(se_NO_CURRENT_TUPLE);

    /* 1. pin record */
    pin_i pin;
    W_DO(pin.pin(tuple->rid(), 0, EX));

    int current_size = pin.body_size();

    /* 2. update record */
    char* pdest = NULL;
    int   bufsz = 0;

    int tsz = tuple->format(pdest, bufsz);
    assert (pdest); // (ip) if NULL invalid

    if (current_size < tsz) {
        w_rc_t rc = db->append_rec(tuple->rid(),
                                   zvec_t(tsz - current_size),
                                   true);
        // on error unpin 
        if (rc!=(RCOK)) pin.unpin();
        W_DO(rc);
    }
    w_rc_t rc = db->update_rec(tuple->rid(), 0, 
                               vec_t(pdest, tsz));
    delete [] pdest;

    /* 3. unpin */
    pin.unpin();
    return (rc);
}



/********************************************************************* 
 *
 *  @fn:    delete_tuple
 *
 *  @brief: Deletes a tuple from a table and the corresponding entries
 *          on all the indexes of the table
 *
 *  @note:  This function should be called in the context of a trx
 *          The passed tuple should be valid.
 *
 *********************************************************************/

w_rc_t  table_desc_t::delete_tuple(ss_m* db, table_row_t* ptuple)
{
    assert (ptuple);

    if (!ptuple->is_rid_valid()) return RC(se_NO_CURRENT_TUPLE);

    /* 1. delete the tuple */
    W_DO(db->destroy_rec(ptuple->rid()));

    /* 2. delete all the corresponding index entries */
    index_desc_t* index = _indexes;
    char* pdest = NULL;
    int   bufsz = 0;
    int key_sz = 0;

    while (index) {
        key_sz = format_key(index, ptuple, pdest, bufsz);
        assert (pdest); // (ip) if NULL invalid key

	W_DO(index->find_fid(db));
	W_DO(db->destroy_assoc(index->fid(),
                               vec_t(pdest, key_sz),
                               vec_t(&(ptuple->_rid), sizeof(rid_t))));

        /* move to next index */
	index = index->next();
    }

    /* invalidate tuple */
    ptuple->set_rid(rid_t::null);

    return RCOK;
}



/* ---------------------- */
/* --- tuple iterator --- */
/* ---------------------- */


w_rc_t  table_desc_t::get_iter_for_file_scan(ss_m* db,
                                             table_scan_iter_impl* &iter)
{
    iter = new table_scan_iter_impl(db, this);
    if (iter->opened()) return RCOK;
    return RC(se_OPEN_SCAN_ERROR);
}


w_rc_t table_desc_t::get_iter_for_index_scan(ss_m* db,
                                             index_desc_t* index,
                                             index_scan_iter_impl* &iter,
                                             scan_index_i::cmp_t c1,
                                             const cvec_t& bound1,
                                             scan_index_i::cmp_t c2,
                                             const cvec_t& bound2,
                                             bool need_tuple)
{
    iter = new index_scan_iter_impl(db, index, need_tuple );
    W_DO(iter->open_scan(db, c1, bound1, c2, bound2, 
                         maxkeysize(index)));

    if (iter->opened())  return RCOK;
    return RC(se_OPEN_SCAN_ERROR);
}



/* ------------------- */
/* --- check index --- */
/* ------------------- */



/******************************************************************** 
 *
 *  @fn:    check_all_indexes
 *
 *  @brief: Check all indexes
 *
 ********************************************************************/

bool table_desc_t::check_all_indexes(ss_m* db)
{
    index_desc_t* index = _indexes;

    TRACE( TRACE_DEBUG, "Checking consistency of the indexes on table (%s)\n",
           _name);

    while (index) {
	w_rc_t rc = check_index(db, index);
	if (rc) {
            TRACE( TRACE_ALWAYS, "Index checking error in (%s) (%s)\n", 
                   name(), index->name());
	    cerr << "Due to " << rc << endl;
	    return (false);
	}
	index = index->next();
    }

    return (true);
}


/********************************************************************* 
 *
 *  @fn:    check_index
 *
 *  @brief: Checks all the values on an index. It first gets the rid from
 *          the table (by scanning) and then probes the index for the same
 *          tuple. It reports error if the two rids do not match.
 *
 *********************************************************************/

w_rc_t table_desc_t::check_index(ss_m* db,
                                 index_desc_t* pindex)
{
    TRACE( TRACE_DEBUG, "Start to check index (%s)\n", pindex->name());

    W_DO(db->begin_xct());

    table_scan_iter_impl* iter;
    W_DO(get_iter_for_file_scan(db, iter));

    bool        eof = false;
    table_row_t tablerow(this);
    W_DO(iter->next(db, eof, tablerow));
    while (!eof) {
        // remember the rid just scanned
        rid_t tablerid = tablerow.rid();
	W_DO(index_probe(db, pindex, &tablerow));
	if (tablerid != tablerow.rid()) {
            TRACE( TRACE_ALWAYS, "Inconsistent index... (%d)-(%d)",
                   tablerid, tablerow.rid());
            return RC(se_INCONSISTENT_INDEX);
	}
	W_DO(iter->next(db, eof, tablerow));
    }

    W_DO(db->commit_xct());

    delete (iter);
    return (RCOK);
}



/* ------------------ */
/* --- scan index --- */
/* ------------------ */



/********************************************************************* 
 *
 *  @fn:    scan_all_indexes
 *
 *  @brief: Scan all indexes
 *
 *********************************************************************/

w_rc_t table_desc_t::scan_all_indexes(ss_m* db)
{
    index_desc_t* index = _indexes;
    while (index) {
	W_DO(scan_index(db, index));
	index = index->next();
    }
    return (RCOK);
}


/********************************************************************* 
 *
 *  @fn:    scan_index
 *
 *  @brief: Iterates over all the values on an index
 *
 *********************************************************************/

w_rc_t table_desc_t::scan_index(ss_m* db, index_desc_t* index)
{
    assert (index);

    TRACE( TRACE_DEBUG, "Scanning index (%s) for table (%s)\n", 
           index->name(), name());

    /* 1. open a index scanner */
    index_scan_iter_impl* iter;

    table_row_t lowtuple(this);
    char* lowkey  = NULL;
    int   lobufsz = 0;
    int   lowsz   = min_key(index, &lowtuple, lowkey, lobufsz);
    assert (lowkey);

    table_row_t hightuple(this);
    char* highkey = NULL;
    int   hibufsz = 0;
    int   highsz  = max_key(index, &hightuple, highkey, hibufsz);
    assert (highkey);

    W_DO(get_iter_for_index_scan(db, index, iter,
				 scan_index_i::ge,
				 vec_t(lowkey, lowsz),
				 scan_index_i::le,
				 vec_t(highkey, highsz),
				 false));
    delete [] highkey;
    delete [] lowkey;

    /* 2. iterate over all index records */
    bool        eof;
    int         count = 0;    
    table_row_t row(this);
    W_DO(iter->next(db, eof, row));
    while (!eof) {	
	pin_i  pin;
	W_DO(pin.pin(row.rid(), 0));
	if (!row.load(pin.body())) return RC(se_WRONG_DISK_DATA);
	pin.unpin();
        row.print_value();

	W_DO(iter->next(db, eof, row));
	count++;
    }
    delete iter;

    /* 3. print out some statistics */
    TRACE( TRACE_DEBUG, "%d tuples found!\n", count);
    TRACE( TRACE_DEBUG, "Scan finished!\n");

    return (RCOK);
}


/* ----------------- */
/* --- debugging --- */
/* ----------------- */

w_rc_t table_desc_t::print_table(ss_m* db)
{
    char   filename[MAX_FILENAME_LEN];
    strcpy(filename, name());
    strcat(filename, ".tbl.tmp");
    ofstream fout(filename);

    W_DO(db->begin_xct());

    table_scan_iter_impl* iter;
    int count = 0;
    W_DO(get_iter_for_file_scan(db, iter));

    bool        eof = false;
    table_row_t row(this);
    W_DO(iter->next(db, eof, row));
    while (!eof) {
        //	row.print_value(fout);
        //        row.print_tuple();
        count++;
	W_DO(iter->next(db, eof, row));
    }

    W_DO(db->commit_xct());
    delete iter;

    fout << "Table : " << _name << endl;
    fout << "Tuples: " << count << endl;

    TRACE( TRACE_DEBUG, "Table (%s) printed (%d) tuples\n",
           _name, count);


    return RCOK;
}


/* For debug use only: print the description for all the field. */
void table_desc_t::print_desc(ostream& os)
{
    os << "Schema for table " << _name << endl;
    os << "Numer of fields: " << _field_count << endl;
    for (int i=0; i<_field_count; i++) {
	_desc[i].print_desc(os);
    }
}







//////////////////////////////////////////////////////////////////////
// 
// class table_desc_t methods 
//
//////////////////////////////////////////////////////////////////////


/* ----------------- */
/* --- formating --- */
/* ----------------- */


/********************************************************************* 
 *
 *  @fn:      format
 *
 *  @brief:   Return a string of the tuple (array of pvalues[]) formatted 
 *            to the appropriate disk format so it can be pushed down to 
 *            data pages in Shore. The size of the data buffer is in 
 *            parameter (bufsz).
 *
 *  @warning: This function should be the inverse of the load() 
 *            function changes to one of the two functions should be
 *            mirrored to the other.
 *
 *  @note:    convert: memory -> disk format
 *
 *********************************************************************/

const int table_row_t::format(char* &dest, int &bufsz)
{
    int var_count  = 0;
    int fixed_size = 0;
    int null_count = 0;
    int tupsize    = 0;


    /* 1. calculate sizes related to the tuple, 
     * - (tupsize)    : total space of the tuple 
     * - (fixed_size) : the space occupied by the fixed length fields 
     * - (null_count) : the number of null flags 
     * - (var_count)  : the number of variable length fields 
     *
     */

    for (int i=0; i<_field_cnt; i++) {

	if (_pvalues[i].is_variable_length()) {
            // If it is of VARIABLE length, then if the value is null
            // do nothing, else add to the total tuple length the (real)
            // size of the value plus the size of an offset.

	    var_count++;
            if (_pvalues[i].is_null()) continue;
            tupsize += _pvalues[i].realsize();
            tupsize += sizeof(offset_t);
	}
	else {            
            // If it is of FIXED length, then increase the total tuple
            // length, as well as, the size of the fixed length part of
            // to tuple by the fixed size of this type of field.

            fixed_size += _pvalues[i].maxsize();
            tupsize += _pvalues[i].maxsize();
        }

	if (_pvalues[i].field_desc()->allow_null())  
            null_count++;
    }

    // In the total tuple length add the size of the bitmap that
    // shows which fields can be NULL 
    if (null_count) tupsize += (null_count >> 3) + 1;


    /* 2. allocate space for formatted data */

    assert (tupsize);
    if ((!dest) || (bufsz < tupsize)) {
        // new larger buffer needs to be allocated
        bufsz = tupsize;
        char* tmp = dest;
	dest = new char[tupsize];
        if (tmp)
            delete [] tmp;
    }
    // in any case, clean up the buffer
    memset (dest, 0, tupsize);


    /* 3. format the data */

    // current offset for fixed length field values
    offset_t fixed_offset = 0;
    if (null_count) fixed_offset = ((null_count-1) >> 3) + 1;
    // current offset for variable length field slots
    offset_t var_slot_offset = fixed_offset + fixed_size; 
    // current offset for variable length field values
    offset_t var_offset = var_slot_offset + sizeof(offset_t)*var_count;

    int null_index = -1;
    // iterate over all fields
    for (int i=0; i<_field_cnt; i++) {

        // Check if the field can be NULL. 
        // If it can be NULL, increase the null_index, and 
        // if it is indeed NULL set the corresponding bit
	if (_pvalues[i].field_desc()->allow_null()) {
	    null_index++;
	    if (_pvalues[i].is_null()) {
		SET_NULL_FLAG(dest, null_index);
	    }
	}

        // Check if the field is of VARIABLE length. 
        // If it is, copy the field value to the variable length part of the
        // buffer, to position  (buffer + var_offset)
        // and increase the var_offset.
	if (_pvalues[i].is_variable_length()) {
	    _pvalues[i].copy_value(dest + var_offset);
            register int offset = _pvalues[i].realsize(); 
	    var_offset += offset;

	    // set the offset 
            offset_t len = offset;
	    memcpy(VAR_SLOT(dest, var_slot_offset), &len, sizeof(offset_t));
	    var_slot_offset += sizeof(offset_t);
	}
	else {
            // If it is of FIXED length, then copy the field value to the
            // fixed length part of the buffer, to position 
            // (buffer + fixed_offset)
            // and increase the fixed_offset
	    _pvalues[i].copy_value(dest + fixed_offset);
	    fixed_offset += _pvalues[i].maxsize();
	}
    }

    return (tupsize);
}


/*********************************************************************
 *
 *  @fn:      load
 *
 *  @brief:   Given a tuple in disk format, read it back into memory
 *            (_pvalues[] array).
 *
 *  @warning: This function should be the inverse of the format() function
 *            changes to one of the two functions should be mirrored to
 *            the other.
 *
 *  @note:    convert: disk -> memory format
 *
 *********************************************************************/

const bool table_row_t::load(const char* data)
{
    int var_count  = 0;
    int fixed_size = 0;
    int null_count = 0;

    /* 1. calculate sizes related to the tuple, 
     * - (fixed_size) : the space occupied by the fixed length fields 
     * - (null_count) : the number of null flags 
     * - (var_count)  : the number of variable length fields 
     *
     */

    for (int i=0; i<_field_cnt; i++) {
	if (_pvalues[i].is_variable_length())
	    var_count++;
	else 
            fixed_size += _pvalues[i].maxsize();

	if (_pvalues[i].field_desc()->allow_null())  
            null_count++;
    }


    /* 2. read the data field by field */

    // current offset for fixed length field values
    offset_t fixed_offset = 0;
    if (null_count) fixed_offset = ((null_count-1) >> 3) + 1;
    // current offset for variable length field slots
    offset_t var_slot_offset = fixed_offset + fixed_size; 
    // current offset for variable length field values
    offset_t var_offset = var_slot_offset + sizeof(offset_t)*var_count;

    int null_index = -1;
    for (int i=0; i<_field_cnt; i++) {

        // Check if the field can be NULL.
        // If it can be NULL, increase the null_index,
        // and check if the bit in the null_flags bitmap is set.
        // If it is set, set the corresponding value in the tuple 
        // as null, and go to the next field, ignoring the rest
	if (_pvalues[i].field_desc()->allow_null()) {
	    null_index++;
	    if (IS_NULL_FLAG(data, null_index)) {
		_pvalues[i].set_null();
		continue;
	    }
	}

        // Check if the field is of VARIABLE length.
        // If it is, copy the offset of the value from the offset part of the
        // buffer (pointed by var_slot_offset). Then, copy that many chars from
        // the variable length part of the buffer (pointed by var_offset). 
        // Then increase by one offset index, and offset of the pointer of the 
        // next variable value
	if (_pvalues[i].is_variable_length()) {
	    offset_t var_len;
	    memcpy(&var_len,  VAR_SLOT(data, var_slot_offset), sizeof(offset_t));
	    _pvalues[i].set_value(data+var_offset, var_len);
	    var_offset += var_len;
	    var_slot_offset += sizeof(offset_t);
	}
	else {
            // If it is of FIXED length, copy the data from the fixed length
            // part of the buffer (pointed by fixed_offset), and the increase
            // the fixed offset by the (fixed) size of the field
	    _pvalues[i].set_value(data+fixed_offset, _pvalues[i].maxsize());
	    fixed_offset += _pvalues[i].maxsize();
	}
    }
    return true;
}



/*********************************************************************
 *
 *  @fn:      load_keyvalue
 *
 *  @brief:   Given a buffer with the representation of the tuple in 
 *            disk format, read back into memory (to _pvalues[] array),
 *            but it reads only the fields that are contained to the
 *            specified index.
 *
 *  @warning: This function should be the inverse of the format_key() 
 *            function changes to one of the two functions should be
 *            mirrored to the other.
 *
 *  @note:    convert: disk -> memory format (for the key)
 *
 *********************************************************************/

const bool table_row_t::load_key(const char* string,
                                 index_desc_t* index)
{
    assert (index);
    assert (string);

    register int offset = 0;
    for (int i=0; i<index->field_count(); i++) {
	register int field_index = index->key_index(i);
        register int size = _pvalues[field_index].maxsize();
        _pvalues[field_index].set_value(string + offset, size);
        offset += size;
    }

    return (true);
}


/* ----------------- */
/* --- debugging --- */
/* ----------------- */

/* For debug use only: print the value of all the fields of the tuple */
void table_row_t::print_value(ostream& os)
{
    //  cout << "Number of fields: " << _field_count << endl;
    for (int i=0; i<_field_cnt; i++) {
	_pvalues[i].print_value(os);
	if (i != _field_cnt) os << DELIM_CHAR;
    }
    os << endl;
}


/* For debug use only: print the tuple */
void table_row_t::print_tuple()
{
    assert (_ptable);
    TRACE( TRACE_DEBUG, "Tuple (%s)\n", _ptable->name());
    
    char* sbuf = NULL;
    int sz = 0;
    for (int i=0; i<_field_cnt; i++) {
        sz = _pvalues[i].get_debug_str(sbuf);
        if (sbuf) {
            TRACE( TRACE_DEBUG, "%d. %s (%d)\n", i, sbuf, sz);
            delete [] sbuf;
            sbuf = NULL;
        }
    }
}
