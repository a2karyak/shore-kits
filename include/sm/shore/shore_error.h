/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_file_desc.h
 *
 *  @brief:  Enumuration of Shore-related errors
 *
 *  @author: Mengzhi Wang, April 2001
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#ifndef __SHORE_ERROR_H
#define __SHORE_ERROR_H

ENTER_NAMESPACE(shore);

/* error codes returned from shore toolkit */

enum {
  se_NOT_FOUND                = 0x810001,
  se_VOLUME_NOT_FOUND         = 0X810002,
  se_INDEX_NOT_FOUND          = 0x810003,
  se_TABLE_NOT_FOUND          = 0x810004,
  se_TUPLE_NOT_FOUND          = 0x810005,
  se_NO_CURRENT_TUPLE         = 0x810006,
  
  se_SCAN_OPEN_ERROR          = 0x810010,
  se_INCONSISTENT_INDEX       = 0x810012,
  se_OPEN_SCAN_ERROR          = 0x810020,
  
  se_LOAD_NOT_EXCLUSIVE       = 0x810040,

  se_WRONG_DISK_DATA          = 0x810050
};



EXIT_NAMESPACE(shore);

#endif /* __SHORE_ERROR_H */
