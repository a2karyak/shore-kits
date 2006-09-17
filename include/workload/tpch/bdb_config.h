
#ifndef _BDB_CONFIG_H
#define _BDB_CONFIG_H



// buffer pool: set size equal to 450 MB -- Maximum is 4GB in 32-bit
// platforms
#define BDB_BUFFER_POOL_SIZE_GB 1 /* 1 GB */
#define BDB_BUFFER_POOL_SIZE_BYTES 600*1024*1024 /* 600MB */

#define BDB_HOME_DIRECTORY  "."
#define BDB_DATA_DIRECTORY  "database"
#define BDB_TEMP_DIRECTORY  "temp"



#endif
