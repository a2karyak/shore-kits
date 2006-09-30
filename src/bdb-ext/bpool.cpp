// -*- mode:c++; c-basic-offset:4 -*-
#include <utility>
#include <list>
#include "util.h"
#include <stdint.h>

struct file_handle {
    int fd;
    bool finished;
};

// pages are identified by file descriptor and page number
typedef pair<int, size_t> page_id;
typedef pair<pthread_mutex_t, pthread_cond_t> synch;

static const int BPOOL_DIRTY   = 0x01; // dirty (must flush on evict)
static const int BPOOL_NEW     = 0x02; // newly created (must flush on evict)
static const int BPOOL_INVALID = 0x04; // eviction in progress (no longer valid)
static const int BPOOL_PAGING  = 0x08; // page swap in progress (will be valid soon)
static const int BPOOL_FREE    = 0x10; // not allocated
static const int BPOOL_DELETED = 0x20; // page has been deleted (no need to flush)

size_t hash(char const* value, size_t len);

template<typename T>
size_t hash(T const &value) {
    hash(static_cast<char const*>(&value), sizeof(value));
}

struct frame_handle;
struct file_handle;
struct region_handle;
struct page_handle;

struct entry {
    struct link {
        entry* next;
        entry* prev;
    };
    
    page_id pid;
    bool L;
    frame* value;

    // protects this entry and the links to its right (ie, next and next->prev)
    pthread_mutex_t lock;
    link hash_chain;
    link lru_chain;
    void lock() {
        thread_mutex_lock(lock);
    }
    void unlock() {
        thread_mutex_unlock(lock);
    }
    void lock_prev() {
        entry* tmp = lru_chain->prev;
        tmp->lock();
        
        // did the entry change while we were trying to get the lock?
        if(tmp != lru_chain->prev) {
            tmp->unlock();
            lock_prev();
        }
    }
};

struct frame {
    file_handle fh;
    page_id pid;
    int ref_count;
    int flags;
    page_handle ph;
    // these are separate bools so that they don't have to be
    // protected with a lock
    bool volatile touched;
    bool volatile L;
};

struct bucket {
    synch _synch;
    frame* first;
    void lock() {
        thread_mutex_lock(_synch.first);
    }
    void unlock() {
        thread_mutex_unlock(_synch.first);
    }
};

struct region {
    synch _synch;
    frame* dummy;
    // regions can be "locked" by critical sections
    operator pthread_mutex_t &() {
        return _synch.first;
    }
};

struct clock {
    size_t size;
    entry* head;
};
class bpool {
    char* data_array;
    frame* frame_array;
    
    int region_count;
    synch* _region_synch;

    int bucket_count;
    bucket* _buckets;

    // Cache lists.
    // use H1/H2 instead of T1/T2 because the pointer is to the head
    // (H) of the list, not the tail (T).
    clock H1;
    clock H2;

    // history lists
    clock B1;
    clock B2;

    static frame const* const PLACEHOLDER;

public:
    char* pin(page_id const &pid);
    char* append(page_id* pid=NULL);
    void unpin(char* page);
    
private:
    // notifies the replacement policy that a frame's ref_count has
    // gone to zero
    void release(frame* fp);

    // 
    frame* allocate(page_id const &pid);

    /**
     * Looks up the index of the frame this page belongs to
     */
    frame* get_frame_for(char const* page) const;
    region &get_region_for(char const* page) const;
    bucket &get_bucket_for(page_id const &pid) const;
};

// connects A (on the left) to B (on the right) and returns A
inline
void connect(entry* A, entry* B) {
    (A->next = B)->prev = A;
}

// moves the head of A to the tail of B
inline
void head2tail(clock &A, clock &B) {
    assert(A.size > 0);

    // check for shortcut case
    if(&A == &B) {
        A.head = A.head->next;
        return;
    }
    
    // snip the head out of A...
    entry* tmp = A.head;
    if(A.size == 1) {
        A.head = NULL;
    }
    else {
        connect(tmp.prev, tmp->next);
        A.head = tmp->next;
    }

    // ...and append it to B
    if(B.size == 0) {
        connect(tmp, tmp);
        B.head = tmp;
    }
    else {
        connect(B.head->prev, tmp);
        connect(tmp, B.head);
    }

    // adjust sizes
    A.size--;
    B.size++;
}

void bpool::adapt_q() {
    if(H1.size + H2.size + B2 - ns >= capacity)
        q = min(q + count, 2*capacity - H1.size);
}

// advances H2 until a stale entry has been found
void bpool::pruneH2() {
    // while reference bit of the head page in H2 is set...
    // tmp should become the new head of H2
    bool count = 0;
    entry* newH2;
    for(newH2=H2.head; newH2->fp->touched; newH2=newH2->next) {
        count++;
        newH2->fp->touched = false;
    }

    // any moves to be made?
    if(count == 0)
        return;

    entry* T2 = H2.head->prev;
    entry* tmp = newH2->prev;

    // was H1 empty?
    if(H1.size == 0) {
        // make the sublist circular and assign it to H1
        connect(tmp, H2.head);
        H1.head = H2.head;
    }
    else {
        // append the sublist to H1 and restore circularity (H1 doesn't change)
        entry* T1 = H1.head->prev;
        connect(T1, H2.head);
        connect(tmp, H1.head);
    }
            
    // finish snipping the sublist out of H2
    if(newH2 == H2.head) {
        assert(count == H2.size);
        
        // H2 now empty
        H2.head = NULL;
    }
    else {
        // make H2 circular again
        connect(T2, newH2);
        H2.head = newH2;
    }
    
    // adjust sizes
    H1.size += count;
    H2.size -= count;

    adapt_q();
}

// advances H1 until a stale entry has been found
void bpool::pruneH1() {
    while(H1.head->L || H1.head->fp->touched) {
        if(H1.head->fp->touched) {
            H1.head->fp->touched = false;
            if(H1.size >= min(p+1, B1.size) && !H1.head->L) {
                H1.head->L = true;
                ns--; nl++;
            }
            head2tail(H1, H1);
        }
        else {
            // move to tail of H2 (either advances or empties H1)
            head2tail(H1, H2);
            
            // adapt q
            q = max(q-1, capacity - H1.size);

            // empty?
            if(H1.size == 0) 
                break;
        }
    }
}

// assumes full cache (H1.size + H2.size == capacity)
frame* bpool::replace(page_id const &pid) {

    if(H2.size > 0)
        pruneH2();
    if(H1.size > 0)
        pruneH1();

    if(H1.size >= max(1,p)) {
        // move head of T1 to B1 MRU
        head2tail(T1, B1);
        ns--;
    }
    else {
        // move head of T2 to B2 MRU
        head2tail(T2, B2);
        nl++;
    }
}

char* bpool::pin(page_id const &pid) {
    bucket &b = get_bucket_for(pid);
    b.lock();
    
    // probe the chain for our frame
    frame* fp = NULL;
    for(fp=b.first; fp && fp->pid != pid; fp=fp->next);

    if(fp) {
        while(fp == PLACEHOLDER)
            thread_cond_wait(b.synch.second);

        b.unlock();
        
        
    }
    else {
    }
}

char* bpool::append(page_id* pid=NULL) {
}
void bpool::unpin(char const* page) {
    frame* fp = get_frame_for(page);
    region* r = get_region_for(page);

    critical_section cs(r);
    if(--fp->ref_count == 0)
        fp->touched = true;
}

