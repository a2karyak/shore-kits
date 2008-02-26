/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_file_desc.h
 *
 *  @brief:  Descriptors for Shore files/indexes, and structures that help in
 *           keeping track of the created files/indexes.
 *
 *  @author: Mengzhi Wang, April 2001
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#ifndef __SHORE_FILE_DESC_H
#define __SHORE_FILE_DESC_H

#include "sm_vas.h"
#include "util.h"
#include "sm/shore/shore_error.h"


#include <list>

using std::list;


ENTER_NAMESPACE(shore);


/******** Exported constants ********/

#define  MAX_FNAME_LEN         40
#define  MAX_TABLENAME_LEN     40
#define  MAX_FIELDNAME_LEN     40


#define  MAX_KEYDESC_LEN       40
#define  MAX_FILENAME_LEN     100

#define  MAX_BODY_SIZE       1024


#define  DELIM_CHAR            '|'

#define  COMMIT_ACTION_COUNT  2000

#define  MIN_SMALLINT     0
#define  MAX_SMALLINT     1<<15
#define  MIN_INT          0
#define  MAX_INT          1<<31
#define  MIN_FLOAT        0
#define  MAX_FLOAT        1<<10



/** @enum file_type_t
 *
 *  @brief a file can be either a regular file, an primary index,
 *  a (secondary) index, etc... 
 *  The list can be expanded.
 */

enum file_type_t {
    FT_REGULAR,
    FT_PRIMARY_IDX,
    FT_IDX,
    FT_NONE
};



/*  --------------------------------------------------------------
 *  @class file_desc_t
 *
 *  @brief class that describes any file (table/index). It provides
 *  the "metadata" methods for its derived classes.
 *
 *  --------------------------------------------------------------- */

class file_desc_t {
protected:
    pthread_mutex_t _fschema_mutex;        // file schema mutex
    char            _name[MAX_FNAME_LEN];  // file name
    int             _field_count;          // # of fields

    vid_t           _vid;                  // volume id
    stid_t          _root_iid;             // root id

    // (ip) We need to add optimistic concurrencny for _vid and _root_iid
    //    pthread_mutex_t _vol_mutex;           // mutex for the vid and root_iid

public:

    stid_t          _fid;                  // physical id of the file


    /* -------------------- */
    /* --- construction --- */
    /* -------------------- */

    file_desc_t(const char* name, int fcnt)
        : _field_count(fcnt), _fid(stid_t::null),
          _vid(vid_t::null), _root_iid(stid_t::null)
    {
        assert (fcnt>0);

        pthread_mutex_init(&_fschema_mutex, NULL);

        // Copy name
	if(strlcpy(_name, name, MAX_FNAME_LEN) >= MAX_FNAME_LEN)
	    throw "file_desc_t::_name too long!\n";
    }

    virtual ~file_desc_t() 
    { 
        pthread_mutex_destroy(&_fschema_mutex);
    }


    /* ---------------------- */
    /* --- access methods --- */
    /* ---------------------- */

    const char*   name() const { return _name; }
    stid_t&       fid() { return (_fid); }
    void          set_fid(stid_t fid) { _fid = fid; }
    vid_t         vid() { return _vid; }   
    stid_t        root_iid() { return _root_iid; }
    int           field_count() const { return _field_count; } 

    bool          is_fid_valid() const { return (_fid != stid_t::null); }
    bool          is_vid_valid() { return (_vid != vid_t::null); }
    bool          is_root_valid() { return (_root_iid != stid_t::null); }

    w_rc_t        find_fid(ss_m* db);
    w_rc_t        find_root_iid(ss_m* db);

    inline w_rc_t check_fid(ss_m* db) {
        if (!is_fid_valid()) {
            if (!is_root_valid())
                W_DO(find_root_iid(db));
            W_DO(find_fid(db));
        }
        return (RCOK);
    }
    
}; // EOF: file_desc_t




/*  --------------------------------------------------------------
 ** @struct file_info_t
 *
 *  @brief Structure that represents a Shore file in a volume. 
 *  This Key information for files goes to the root btree index.
 * 
 *  --------------------------------------------------------------- */

class file_info_t {
private:
    char              _fname[MAX_FNAME_LEN];        // file name
    file_type_t        _ftype;        // {regular,primary idx, idx, ...}
    std::pair<int,int> _record_size;  // size of each column of the file record

    rid_t        _first_rid;   // first record
    rid_t        _cur_rid;     // current tuple id 

public:

    stid_t             _fid;          // using physical interface

    // Constructor
    file_info_t(const stid_t& fid,
                const char* fname, 
                const file_type_t ftype = FT_REGULAR)
        : _fid(fid), _ftype(ftype)
    {
	if(strlcpy(_fname, fname, MAX_FNAME_LEN) >= MAX_FNAME_LEN)
	    throw "file_info_t::_fname too long!\n";
    }

    // TODO REMOVE no argument consturctor
    file_info_t() : _ftype(FT_REGULAR) { }

    ~file_info_t() { }


    // Access methods
    void set_fid(const stid_t& fid) { _fid = fid; }
    stid_t& fid() { return (_fid); }
    void set_ftype(const file_type_t& ftype) { _ftype = ftype; }
    file_type_t& ftype() { return (_ftype); }
    void set_first_rid(const rid_t arid) { _first_rid = arid; }
    rid_t& first_rid() { return (_first_rid); }
    void set_curr_rid(const rid_t arid) { _cur_rid = arid; }
    rid_t& curr_rid() { return (_cur_rid); }
    void set_record_size(const std::pair<int,int> apair) { _record_size = apair; }
    std::pair<int,int> record_size() { return (_record_size); }

}; // EOF: file_info_t


typedef std::list<file_info_t> file_list;



EXIT_NAMESPACE(shore);

#endif /* __SHORE_FILE_DESC_H */
