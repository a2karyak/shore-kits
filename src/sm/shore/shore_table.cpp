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

using namespace shore;




/* ----------------------------------- */
/* --- @class table_desc_t methods --- */
/* ----------------------------------- */


#define VAR_SLOT(start, offset)   ((offset_t*)((start)+(offset)))
#define SET_NULL_FLAG(start, offset)                            \
    (*(char*)((start)+((offset)>>3))) &= (1<<(offset%8))
#define IS_NULL_FLAG(start, offset)                     \
    (*(char*)((start)+((offset)>>3)))&(1<<(offset%8))



/*==================== Formating ====================*/


/** @fn    format
 *
 *  @brief For a given tuple in memory (data in _value array), we
 *         format the tuple in disk format so that we can push it 
 *         down to data pages in Shore.
 *
 *  @note  convert: memory -> disk format
 */

const char* table_desc_t::format()
{
    int i;
    int var_count = 0;
    int fixed_size = 0;
    int null_count = 0;

    /* 1. calculate the total space occupied by the fixed
     * length fields and null flags.
     */
    for (i=0; i<_field_count; i++) {
	if (_desc[i].is_variable_length()) {
	    var_count++;
	}
	else fixed_size += _desc[i].maxsize();
	if (_desc[i].allow_null())  null_count++;
    }

    /* 2. allocate space for formatted data */
    if (!_formatted_data) {
	_formatted_data = new char[maxsize()+var_count*sizeof(offset_t)+null_count];
    }

    /* 3. format the data */

    // current offset for fixed length field values
    offset_t    fixed_offset = 0;
    if (null_count) fixed_offset = ((null_count-1) >> 3) + 1;
    // current offset for variable length field slots
    offset_t    var_slot_offset = fixed_offset + fixed_size; 
    // current offset for variable length field values
    offset_t    var_offset = var_slot_offset + sizeof(offset_t)*var_count;

    int    null_index = -1;
    for (i=0; i<_field_count; i++) {
	if (_desc[i].allow_null()) {
	    null_index++;
	    if (_desc[i].is_null()) {
		SET_NULL_FLAG(_formatted_data, null_index);
	    }
	}

	if (_desc[i].is_variable_length()) {
	    _desc[i].copy_value(_formatted_data + var_offset);
	    var_offset += _desc[i].realsize();

	    // set the offset
	    offset_t   len = _desc[i].realsize();
	    memcpy(VAR_SLOT(_formatted_data, var_slot_offset), &len, sizeof(offset_t));
	    var_slot_offset += sizeof(offset_t);
	}
	else {
	    _desc[i].copy_value(_formatted_data + fixed_offset);
	    fixed_offset += _desc[i].maxsize();
	}
    }

    return _formatted_data;
}


/** @fn    load
 *
 *  @brief Given a tuple in disk format, we read it back into memory
 *         (_value array).
 *
 *  @note  convert: disk -> memory format
 */

bool table_desc_t::load(const char* data)
{
    int i;
    int var_count = 0;
    int fixed_size = 0;
    int null_count = 0;

    /* 1. calculate the total space occupied by the fixed
     * length fields.
     */
    for (i=0; i<_field_count; i++) {
	if (_desc[i].is_variable_length()) {
	    var_count++;
	}
	else fixed_size += _desc[i].maxsize();
	if (_desc[i].allow_null())  null_count++;
    }


    /* 2. read the data field by field */

    // current offset for fixed length field values
    offset_t    fixed_offset = 0;
    if (null_count) fixed_offset = ((null_count-1) >> 3) + 1;
    // current offset for variable length field slots
    offset_t    var_slot_offset = fixed_offset + fixed_size; 
    // current offset for variable length field values
    offset_t    var_offset = var_slot_offset + sizeof(offset_t)*var_count;

    int null_index = -1;
    for (i=0; i<_field_count; i++) {
	if (_desc[i].allow_null()) {
	    null_index++;
	    if (IS_NULL_FLAG(data, null_index)) {
		_desc[i].set_null();
		continue;
	    }
	}

	if (_desc[i].is_variable_length()) {
	    offset_t var_len;
	    memcpy(&var_len,  VAR_SLOT(data, var_slot_offset), sizeof(offset_t));
	    _desc[i].set_value(data+var_offset,
			       var_len);
	    var_offset += var_len;
	    var_slot_offset += sizeof(offset_t);
	}
	else {
	    _desc[i].set_value(data+fixed_offset,
			       _desc[i].maxsize());
	    fixed_offset += _desc[i].maxsize();
	}
    }
    return true;
}


bool   table_desc_t::load_keyvalue(const unsigned char* string,
                                   index_desc_t* index)
{
    int offset = 0;
    for (int i=0; i<index->field_count(); i++) {
	int field_index = index->key_index(i);
	if (_desc[field_index].is_variable_length()) {
	    _desc[field_index].set_value(string + offset,
					 _desc[field_index].realsize());
	    offset += _desc[field_index].maxsize();
	}
	else {
	    int size = _desc[field_index].maxsize();
	    _desc[field_index].set_value(string + offset,
					 size);
	    offset += size;
	}
    }
    return true;
}



/*==================== bulkload utilities ====================*/

w_rc_t    table_desc_t::load_table_from_file(ss_m* db)
{
    cout << "Loading " << _name << " table ..." << endl;

    char   filename[MAX_FILENAME_LEN];
    strcpy(filename, name());
    strcat(filename, ".tbl");
    ifstream   fin(filename);
    
    W_DO(db->begin_xct());

    /* 1. check if the file already exists */
    w_rc_t  result = find_fid(db);
    
    if (result == RC(se_TABLE_NOT_FOUND)) {
	/* create a new file if it does not exist */
	W_DO(db->create_file(vid(), _fid, smlevel_3::t_regular));

	file_info_t info;
	info.set_fid(_fid);
	info.set_ftype(FT_REGULAR);
	smsize_t  infosize = sizeof(info);


        /** (ip) Trying to remove warning of inaccessible copy constructor */
        /**      Converting the first pointer to (const void*) */
        /**      vec_t((const void*, size_t)) */

        const vec_t a = vec_t(_name, strlen(_name));
        vec_t c = vec_t(_name, strlen(_name));
        const vec_t b = vec_t(&info, infosize);

	W_DO(ss_m::create_assoc(root_iid(),
				vec_t(_name, strlen(_name)),
				vec_t(&info, infosize)));               
// 				vec_t(_name, strlen(_name)),
// 				vec_t(&info, infosize)));
    }
    else if (result != RCOK) return result;

    /* 2. append the tuples */
    append_file_i  file_append(fid());
    
    int tuple_count = 0;
    while (!fin.eof()) {
	for (int i=0; i<_field_count; i++) {
	    char delim = DELIM_CHAR;
	    if (i == _field_count-1) delim = '\n';
	    if (!_desc[i].load_value_from_file(fin, delim))
		goto end_of_input_file;
	    fin.get(delim);
	}

	tuple_count++;
	W_DO(file_append.create_rec(vec_t(), 0,
				    vec_t(format(), size()),
				    _rid));

	if ((tuple_count % COMMIT_ACTION_COUNT) == 0) {
	    W_DO(db->commit_xct());
	    if (ss_m::num_active_xcts() != 0)
		return RC(se_LOAD_NOT_EXCLUSIVE);
	    W_DO(db->begin_xct());
	}
    }

 end_of_input_file:

    W_DO(db->commit_xct());

    cout << "# of records inserted: " << tuple_count << endl;
    cout << "Building indices ... " << endl;

    /* 3. update the index structures */
    return bulkload_index(db);
}


w_rc_t table_desc_t::bulkload_index(ss_m* db,
                                    index_desc_t * index)
{
    cout << "Starting to build index: " << index->name() << endl;
    
    W_DO(db->begin_xct());

    /* 0. the index file should already been created by create_table() */
    
    /* 1. insert the indices for all tuples in the table */
    table_scan_iter_impl* iter;
    W_DO(get_iter_for_file_scan(db, iter));

    int   tuple_count = 0;
    bool  eof;
    W_DO(iter->next(db, eof));

    while (!eof) {
	W_DO(db->create_assoc(index->fid(),
                              vec_t(format_key(index), key_size(index)),
                              vec_t(&_rid, sizeof(rid_t))));
	W_DO(iter->next(db, eof));
	tuple_count++;

	if ( tuple_count % 1000 == 0 ) { 
            cerr << "bulkload_index(" << index->name() << "): " << tuple_count << endl;
	}
    }

    delete iter;
    W_DO(db->commit_xct());

    return RCOK;
}


w_rc_t table_desc_t::bulkload_index(ss_m* db,
                                    const char* name)
{
    index_desc_t* index = (_indexes ? _indexes->find_by_name(name) : NULL);

    if (index)
        return bulkload_index(db, index);
    return RC(se_INDEX_NOT_FOUND);
}


w_rc_t table_desc_t::bulkload_index(ss_m* db)
{
    cout << "Start building indices for: " << _name << endl;
  
    index_desc_t* index = _indexes;

    while (index) {
	// build one index at each iteration.
	W_DO(bulkload_index(db, index));
	index = index->next();
    }

    cout << "End building indices" << endl;
    return RCOK;
}


/*=========================== access methods =========================*/

w_rc_t  table_desc_t::index_probe(ss_m* db,
                                  index_desc_t* index,
                                  lock_mode_t lmode)
{
    bool     found;
    smsize_t len = sizeof(rid_t);

    /* 1. find the index structure */
    W_DO(index->check_fid(db));

    /* 2. find the tuple in the B+tree */
    W_DO(ss_m::find_assoc(index->fid(),
			  vec_t(format_key(index), key_size(index)),
			  &_rid,
			  len,
			  found));
    if (!found) return RC(se_TUPLE_NOT_FOUND);

    /* 3. read the tuple */
    pin_i   pin;
    W_DO(pin.pin(rid(), 0, lmode));
    if (!load(pin.body())) return RC(se_WRONG_DISK_DATA);
    pin.unpin();
  
    return RCOK;
}



/*=================== create physical table =========================*/


w_rc_t table_desc_t::create_table(ss_m* db)
{
    if (!is_vid_valid() || !is_root_valid())
	W_DO(find_root_iid(db));

    /* create the table */
    file_info_t   file;
    W_DO(db->create_file(_vid, _fid, smlevel_3::t_regular));

    /* add table entry to the metadata tree */
    file.set_ftype(FT_REGULAR);
    file.set_fid(_fid);
    W_DO(ss_m::create_assoc(root_iid(),
			    vec_t(name(), strlen(name())),
			    vec_t(&file, sizeof(file_info_t))));
    

    /* create the indexes */
    index_desc_t* index = _indexes;
    while (index) {
	stid_t  iid;
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
    
    return RCOK;
}


//=================== tuple manipulation =========================


w_rc_t table_desc_t::add_tuple(ss_m* db)
{
    /* 1. find the file */
    W_DO(check_fid(db));

    /* 2. append the tuple */
    W_DO(db->create_rec(fid(), vec_t(), size(),
                        vec_t(format(), size()),
                        _rid));

    /* 3. update the indexes */
    index_desc_t* index = _indexes;
    while (index) {
	W_DO(index->find_fid(db));
	W_DO(db->create_assoc(index->fid(),
                              vec_t(format_key(index), key_size(index)),
                              vec_t(&_rid, sizeof(rid_t))));

        /* move to next index */
	index = index->next();
    }

    return RCOK;
}


w_rc_t table_desc_t::update_tuple(ss_m * db)
{
    if (!is_rid_valid()) return RC(se_NO_CURRENT_TUPLE);

    /* 1. pin record */
    pin_i   pin;
    W_DO(pin.pin(rid(), 0, EX));

    int current_size = pin.body_size();

    /* 2. update record */
    if (current_size < size()) {
        w_rc_t rc = db->append_rec(rid(),
                                   zvec_t(size()-current_size),
                                   true);
        if (rc!=RCOK) pin.unpin();
        W_DO(rc);
    }
    w_rc_t rc = db->update_rec(rid(), 0, vec_t(format(), size()));

    /* 3. unpin */
    pin.unpin();
    return rc;
}


w_rc_t  table_desc_t::delete_tuple(ss_m* db)
{
    if (!is_rid_valid()) return RC(se_NO_CURRENT_TUPLE);

    /* 1. delete the tuple */
    W_DO(db->destroy_rec(rid()));
    _rid = rid_t::null;

    /* 2. delete the index entries */
    index_desc_t* index = _indexes;
    while (index) {
	W_DO(index->find_fid(db));
	W_DO(db->destroy_assoc(index->fid(),
                               vec_t(format_key(index), key_size(index)),
                               vec_t(&_rid, sizeof(rid_t))));

        /* move to next index */
	index = index->next();
    }

    return RCOK;
}


/*======================== tuple iterator ===========================*/


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
    W_DO( iter->open_scan(db, c1, bound1, c2, bound2, 
                          maxkeysize(index)));

    if (iter->opened())  return RCOK;
    return RC(se_OPEN_SCAN_ERROR);
}



/*======================== check index ===========================*/


bool table_desc_t::check_index(ss_m* db)
{
    index_desc_t* index = _indexes;

    cout << "Checking consistency of the indices on table " << _name << endl;

    while (index) {
	w_rc_t rc = check_index(db, index);
	if (rc) {
	    cout << "Index checking error in " << name() << " " << index->name() << endl;
	    cout << "Due to " << rc << endl;
	    return false;
	}
	index = index->next();
    }
    return true;
}


w_rc_t table_desc_t::check_index(ss_m* db,
                                 index_desc_t* pindex)
{
    cout << "Start to check index " << pindex->name() << endl;

    // XCT_BEGIN
    W_DO(db->begin_xct());

    table_scan_iter_impl* iter;
    W_DO(get_iter_for_file_scan(db, iter));

    bool  eof = false;
    W_DO(iter->next(db, eof));
    while (!eof) {
	rid_t tuple_rid = rid();
	W_DO(index_probe(db, pindex));
	if (tuple_rid != rid()) {
            cerr << " Inconsistent index... " << endl;
            return RC(se_INCONSISTENT_INDEX);
	}

	W_DO(iter->next(db, eof));
    }

    W_DO(db->commit_xct());
    // XCT_COMMIT

    delete iter;
    return RCOK;
}



/*======================== scan index ===========================*/

/** scans all indexes */
w_rc_t table_desc_t::scan_index(ss_m* db)
{
    index_desc_t* index = _indexes;
    while (index) {
	W_DO(scan_index(db, index));
	index = index->next();
    }
    return RCOK;
}


/** @fn    scan_index
 *
 *  @brief Iterates over all the values on an index
 */

w_rc_t table_desc_t::scan_index(ss_m* db, index_desc_t* index)
{
    cout << "Scanning index " << index->name() << " for table " << name() << endl;

    /* 1. open a index scanner */
    index_scan_iter_impl* iter;
    char* high_key = new char [maxkeysize(index)];
    char* low_key = new char [maxkeysize(index)];
    memcpy(high_key, max_key(index), key_size(index));
    memcpy(low_key, min_key(index), key_size(index));
    W_DO(get_iter_for_index_scan(db, index, iter,
				 scan_index_i::ge,
				 vec_t(low_key, key_size(index)),
				 scan_index_i::le,
				 vec_t(high_key, key_size(index)),
				 false));
    delete high_key;
    delete low_key;

    /* 2. iterate over all index records */
    bool  eof;
    int   count = 0;
    W_DO(iter->next(db, eof));

    while (!eof) {
	print_value();
	pin_i  pin;
	W_DO(pin.pin(rid(), 0));
	if (!load(pin.body())) return RC(se_WRONG_DISK_DATA);
	pin.unpin();

	W_DO(iter->next(db, eof));
	count++;
    }
    delete iter;

    /* 3. print out some statistics */
    cout << count << " tuples found!" << endl;
    cout << "Scan finished!" << endl;

    return RCOK;
}



/*======================== debugging functions ===========================*/


w_rc_t table_desc_t::print_table(ss_m* db)
{
    char   filename[MAX_FILENAME_LEN];
    strcpy(filename, name());
    strcat(filename, ".tbl");
    ofstream fout(filename);

    W_DO(db->begin_xct());

    table_scan_iter_impl* iter;
    W_DO(get_iter_for_file_scan(db, iter));

    bool  eof;
    W_DO(iter->next(db, eof));
    while (!eof) {
	print_value(fout);
	W_DO(iter->next(db, eof));
    }

    W_DO(db->commit_xct());
    delete iter;

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


/* For debug use only: print the value for all the fields. */
void table_desc_t::print_value(ostream& os)
{
    //  cout << "Numer of fields: " << _field_count << endl;
    for (int i=0; i<_field_count; i++) {
	_desc[i].print_value(os);
	if (i != _field_count) os << DELIM_CHAR;
    }
    os << endl;
}
