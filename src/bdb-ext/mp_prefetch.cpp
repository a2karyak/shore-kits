#include "util.h"

#include <sys/mman.h>
#include <cerrno>
#include <cassert>
#include <vector>
#include <utility>
#include <algorithm>

static const size_t PAGE_SIZE = 4096;

struct stream_key {
    int fd;
    size_t block;
    stream_key(int fd_, size_t b)
        : fd(fd_), block(b)
    {
    }
    bool operator ==(stream_key const &other) const {
        return fd == other.fd && block == other.block;
    }
    bool operator !=(stream_key const &other) const {
        return !(*this == other);
    }
};

typedef std::pair<stream_key, char*> cache_entry;
typedef std::vector<cache_entry> disk_cache;
static char* buf;
static pthread_mutex_t cache_lock = thread_mutex_create();
static size_t cache_size=0;

static size_t stream_size; // in pages

static disk_cache cache;

static const int LOCKED = -1;
static const stream_key dummy_key(1, 1);

extern "C"
int disk_cache_invalidate(int fd, size_t page, size_t size) {
    assert(size == PAGE_SIZE);
    return 0;
}

/**
 *  @brief A critical section manager. Locks and unlocks the specified
 *  mutex upon construction and destruction respectively.
 */

extern "C"
int disk_cache_fetch(void* dest, int fd, size_t page, size_t size) {
    assert(size == PAGE_SIZE);
    int block = page & (stream_size-1);
    int block_offset = page - block;
    int byte_offset = block_offset*PAGE_SIZE;
    
    critical_section_t cs(cache_lock);
    
    // which block does this page belong to?
    stream_key request(fd, block);
    
    // hit?
    int i;
    for(i=0; i < (int) cache.size() && cache[i].first != request; i++);
    if(i < (int) cache.size()) {

        // bubble the entry to the front to preserve LRU ordering
        if(block_offset < (int) stream_size-1) {
            while(i--)
                std::swap(cache[i], cache[i+1]);
        }
    }
    
    // miss.
    else {
        // find the oldest unlocked entry to use as a victim
        for(i=cache.size(); i-- > 0 && cache[i].first.fd == LOCKED; );
        if(i < 0) {
            // too many prefetches in progress. Drop this one on the floor.
            return -1;
        }

        // invalidate and mark it, then use its buffer
        stream_key marker(LOCKED, i); 
        cache[i].first = marker;
        char* buf = cache[i].second;
    
        // release the lock during the I/O
        int target_count = stream_size*PAGE_SIZE;
        cs.exit();
        int count = pread(fd, buf, target_count, block*PAGE_SIZE);

        // re-acquire the lock, then find our entry again (LRU could have
        // moved it while we were gone)
        cs.enter(cache_lock);
        for(i=cache.size(); i-- > 0 && cache[i].first != marker; );
        assert(i >= 0);
    
        // sanity check
        if(count != target_count) {
            // TODO: mask out invalid pages instead of failing outright
            // set the entry to a bogus value so it gets replaced
            cache[i].first = dummy_key;
            // TODO: bubble it to the end so it gets replaced first
            return -2;
        }

        // success! update the stream key
        cache[i].first = request;

        // move this stream to the middle of the LRU stack so it
        // doesn't get replaced too quickly
        for(int j=i; --j > (int) cache.size()/2; )
            std::swap(cache[i], cache[i+1]);
    }

    // copy the data across and we're done
    memcpy(dest, &cache[i].second[byte_offset], PAGE_SIZE);
    return 0;
}
        
extern "C"
int disk_cache_init(size_t size, int streams) {
    cache_size = size;
    // must be at least 1 stream
    assert(streams >= 1);
    
    stream_size = cache_size/streams/PAGE_SIZE;
    // each stream must contain an integral number of pages...
    assert(stream_size*streams*PAGE_SIZE == cache_size);
    // ...that is greater than 1 ...
    assert(stream_size > 1);
    // ...and a power of two...
    assert((stream_size & -stream_size) == stream_size);

    // allocate the memory
    buf = (char*) mmap(0, size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_POPULATE, 0, 0);
    if(buf == MAP_FAILED) {
        perror("init_disk_cache");
        return errno;
    }

    // populate the cache with unaligned 
    for(int i=0; i < streams; i++)
        cache.push_back(std::make_pair(dummy_key, &buf[i*stream_size*PAGE_SIZE]));
    
    return 0;
}

extern "C"
int disk_cache_destroy() {
    if(cache_size != 0) {
        if(munmap(buf, cache_size)) {
            perror("destroy_disk_cache");
            return errno;
        }
    }

    return 0;
}

