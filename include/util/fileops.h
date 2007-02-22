/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file fileops.h
 *
 *  @brief Exports common file checking operations.
 *
 *  @author Naju Mancheril (ngm)
 *
 *  @bug See fileops.c.
 */
#ifndef _FILEOPS_H
#define _FILEOPS_H


/* exported constants */

#define FILEOPS_MAX_PATH_SIZE            1024
#define FILEOPS_ERROR_NOT_FOUND         -1
#define FILEOPS_ERROR_NOT_DIRECTORY     -2
#define FILEOPS_ERROR_PATH_TOO_LONG     -3
#define FILEOPS_ERROR_PERMISSION_DENIED -4


/* exported functions */

int fileops_check_file_exists    (const char* path);
int fileops_check_file_directory (const char* path);
int fileops_check_file_readable  (const char* path);
int fileops_check_file_writeable (const char* path);
int fileops_check_file_executable(const char* path);

int fileops_check_directory_accessible(const char* path);
int fileops_check_file_creatable      (const char* path);

int fileops_parse_parent_directory(char* dst, int dst_size, const char* src);


#endif
