#include "db_cxx.h"
#include <cstdio>

static const size_t PAGE_SIZE = 512;

int main() {
    DbEnv env(0);
    // use a small number of pages 
    env.set_cachesize(0, PAGE_SIZE*4, 0);
    env.set_tmp_dir("/home/ryan/bdb/tmp");
    env.set_data_dir("/home/ryan/bdb/data");
    env.set_msgfile(stdout);
    env.open(NULL, DB_CREATE | DB_PRIVATE | DB_THREAD | DB_INIT_MPOOL, 0);

    
    // open a file in the pool, set to delete on finish
    DbMpoolFile* pool;
    env.memp_fcreate(&pool, 0);
    //    pool->set_flags(DB_MPOOL_UNLINK, true);
    pool->open(NULL, DB_CREATE, 0644, PAGE_SIZE);

    printf("<<<< Starting stats:\n");
    env.memp_stat_print(DB_STAT_ALL);

    const char* actions = "wwwwRwRwRRRR";
    bool done = false;
    db_pgno_t rpn=1, wpn;
    void* p;
    while(!done) {
        switch(*(actions++)) {
        case 'r':
        case 'R':
            pool->get(&rpn, 0, &p);
            printf("\n\n<<<< Read page %d:\n", rpn);
            pool->put(p, DB_MPOOL_CLEAN | DB_MPOOL_DISCARD);
            rpn++;
            break;
        case 'w':
        case 'W':
            pool->get(&wpn, DB_MPOOL_NEW, &p);
            printf("\n\n<<<< Wrote page %d:\n", wpn);
            memset(p, 0, PAGE_SIZE);
            memcpy(p, &wpn, sizeof(wpn));
            pool->put(p, DB_MPOOL_DIRTY);
            break;
        default:
            done = true;
        }

        env.memp_stat_print(0);
    }

    pool->close(0);
    env.close(0);
    return 0;
}
