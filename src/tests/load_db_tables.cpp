#include <cstdio>
#include "engine/thread.h"
#include "workload/tpch/tpch_db_load.h"
#include "workload/tpch/tpch_db.h"

int main() {
    thread_init();
    if(db_open(DB_CREATE))
        printf("Tables opened\n");
    
    if (db_load("tbl")) 
        printf("Tables populated!\n");

    db_close();
    return 0;
}
