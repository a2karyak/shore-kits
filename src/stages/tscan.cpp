/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages/tscan.h"
#include <unistd.h>



const c_str tscan_packet_t::PACKET_TYPE = "TSCAN";



const c_str tscan_stage_t::DEFAULT_STAGE_NAME = "TSCAN_STAGE";



#define KB 1024
#define MB 1024 * KB

/**
 *  @brief BerkeleyDB cannot read into page_t's. Allocate a large blob
 *  and do bulk reading. The blob must be aligned for int accesses and
 *  a multiple of 1024 bytes long.
 */
const size_t tscan_stage_t::TSCAN_BULK_READ_BUFFER_SIZE=4*KB*KB;

struct buffer {
    volatile bool full;
    pthread_cond_t cond;
    dbt_guard_t data;
    buffer()
        : full(false),
          cond(thread_cond_create()),
          data(tscan_stage_t::TSCAN_BULK_READ_BUFFER_SIZE)
    {
    }
};

// forward declarations
class slave_thread;
template <class Action>
void double_buffer(slave_thread &slave, Action action, bool sense);

struct ReadAction {
    stage_t::adaptor_t* adaptor;
    ReadAction(stage_t::adaptor_t* a)
        : adaptor(a)
    {
    }
    int operator()(buffer &b) {

        // iterate over the blob we read and output the individual
        // tuples
        Dbt key, data;
        DbMultipleKeyDataIterator it = b.data.get();
        for (int tuple_index = 0; it.next(key, data); tuple_index++) 
            adaptor->output(data);
            
        return 0;
    }
};

// created by a master thread, destroyed/joined when it goes out of scope
struct slave_thread : thread_t {
    pthread_t tid;
    cursor_guard_t cursor;
    pthread_mutex_t lock;
    volatile bool done;
    buffer buffers[2];
    
    struct WriteAction {
        Dbc* cursor;
        WriteAction(Dbc* c)
            : cursor(c)
        {
        }
        int operator()(buffer &b) {
            Dbt bulk_key;
            return cursor->get(&bulk_key, b.data, DB_MULTIPLE_KEY | DB_NEXT);
        }
    };

    virtual void* run() {
        double_buffer(*this, WriteAction(cursor), true);
        return NULL;
    }
        
    slave_thread(Db* db)
        : thread_t("TSCAN slave"),
          tid(0),
          cursor(db),
          lock(thread_mutex_create()),
          done(false)
    {
        _delete_me = false;
    }
    // MUST be called while holding the mutex!
    void signal_done() {
        done = true;
        pthread_cond_signal(&buffers[0].cond);
        pthread_cond_signal(&buffers[1].cond);
    }
    
    ~slave_thread() {
        critical_section_t cs(lock);
        signal_done();
        cs.exit();
        pthread_join(tid, NULL);
    }
};

template <class Action>
void double_buffer(slave_thread &slave, Action action, bool sense) {
    // start with buffer 0
    int err = 0;
    critical_section_t cs(slave.lock);
    for(int which=0; !err; which = ~which) {
        // 'which' alternates between 0 and -1, so 'which+1' follows 1, 0, 1, 0...
        buffer &b = slave.buffers[which+1];
        while(b.full == sense && !slave.done)
            pthread_cond_wait(&b.cond, &slave.lock);
            
        if(b.full == sense)
            break;

        // leave the critical section before performing the action
        cs.exit();
        err = action(b);
        
        // re-enter after writing, then set the full flag
        cs.enter(slave.lock);
        if(err)
            break;

        b.full = sense;
        pthread_cond_signal(&b.cond);
    }

    // must be DB_NOTFOUND or no error
    slave.signal_done();
    assert(err == 0 || err == DB_NOTFOUND);
}

/**
 *  @brief Read the specified table.
 *
 *  @return 0 on success. Non-zero on unrecoverable error. The stage
 *  should terminate all queries it is processing.
 */

void tscan_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;
    tscan_packet_t* packet = (tscan_packet_t*)adaptor->get_packet();

    // create a slave thread and run it
    slave_thread slave(packet->_db);
    slave.tid = thread_create(&slave);

    // now process the data the slave buffers up
    double_buffer(slave, ReadAction(adaptor), false);
}
