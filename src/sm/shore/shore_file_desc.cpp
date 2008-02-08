/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_file_desc.cpp
 *
 *  @brief Implementation of base class for describing
 *         Shore files (table/index).
 *
 *  @author: Mengzhi Wang, April 2001
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#include "sm/shore/shore_file_desc.h"

using namespace shore;


/* ---------------------------------- */
/* --- @class file_desc_t methods --- */
/* ---------------------------------- */


/** @fn:    find_root_iid 
 *
 *  @brief: Sets the volume fid and root iid.
 *
 *  @note:  Since it sets the static variables this function
 *          most likely will be called only once.
 */
 
w_rc_t file_desc_t::find_root_iid(ss_m* db)
{
    // (ip) Disabled returns the single vid

#if 0 
    vid_t  vid;    
    vid_t* vid_list = NULL;;
    u_int  vid_cnt = 0;
    
    assert (false); // TODO (ip) defined device name
    W_DO(db->list_volumes("device_name", vid_list, vid_cnt));

    if (vid_cnt == 0)
        return RC(se_VOLUME_NOT_FOUND);

    vid = vid_list[0];
    delete [] vid_list;
    _vid = vid;
#else
    // set the two static variables
    _vid = 1; /* (ip) explicitly set volume id = 1 */
#endif

    W_DO(ss_m::vol_root_index(_vid, _root_iid));

    return RCOK;
}


/** @fn:    find_fid 
 *
 *  @brief: Sets the file fid given the file name.
 */

w_rc_t file_desc_t::find_fid(ss_m* db)
{
    // if valid fid don't bother to lookup
    if (is_fid_valid())
        return RCOK;

    file_info_t   info;
    bool          found = false;
    smsize_t      infosize = sizeof(file_info_t);

    if (!is_root_valid()) W_DO(find_root_iid(db));
    
    W_DO(ss_m::find_assoc(root_iid(),
			  vec_t(_name, strlen(_name)),
			  &info, infosize,
			  found));
    _fid = info.fid();
    
    if (!found) {
        cerr << "Problem finding table " << _name << endl;
        return RC(se_TABLE_NOT_FOUND);
    }

    return RCOK;
}
