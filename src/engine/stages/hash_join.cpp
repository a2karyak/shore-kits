
/* hash_join_stage.cpp */
/* Implementation of the HASH_JOIN operator */
/* History: 
   3/6/2006: Uses the outtup variable of the output_tup_t, instead of the data.
*/


#include "stages/hash_join.h"
#include "functors.h"
#include "util/fnv.h"
#include <cstring>
#include <algorithm>
#include "util/tmpfile.h"
#include "dispatcher.h"

const char *hash_join_packet_t::PACKET_TYPE = "Hash Join";

struct hash_join_stage_t::hash_key_t {
    size_t _len;
    hash_key_t(size_t len)
        : _len(len)
    {
    }
    size_t operator()(const char *key) const {
        return fnv_hash(key, _len);
    }
};

template <bool left>
struct hash_join_stage_t::extract_key_t {
    mutable tuple_join_t *_join;
    mutable array_guard_t<char> _k1;
    mutable array_guard_t<char> _k2;
    mutable tuple_t _tuple;

    extract_key_t(tuple_join_t *join)
        : _join(join),
          _k1(new char[_join->key_size()]),
          _k2(new char[_join->key_size()]),
          _tuple(NULL, left? _join->left_tuple_size() : _join->right_tuple_size())
    {
    }
    extract_key_t(const extract_key_t &other)
        : _join(other._join),
          _k1(new char[_join->key_size()]),
          _k2(new char[_join->key_size()]),
          _tuple(NULL, left? _join->left_tuple_size() : _join->right_tuple_size())
    {
    }
    extract_key_t &operator =(const extract_key_t &other) {
        _join = other._join;
        _k1 = new char[_join->key_size()];
        _k2 = new char[_join->key_size()];
        _tuple = other._tuple;
    }
    char* const operator()(char * value) const {
        // alternate keys so we don't clobber a live value
        array_guard_t<char> tmp = _k1.release();
        _k1 = _k2.release();
        _k2 = tmp.release();

        // extract the key and return it
        _tuple.data = value;
        if(left)
            _join->get_left_key(_k1, _tuple);
        else
            _join->get_right_key(_k1, _tuple);
        
        return _k1;
    }
};

struct hash_join_stage_t::equal_key_t {
    size_t _len;
    equal_key_t(size_t len)
        : _len(len)
    {
    }
    bool operator()(const char *k1, const char *k2) const {
        return !memcmp(k1, k2, _len);
    }
};

int hash_join_stage_t::test_overflow(int partition) {
    partition_t &p = partitions[partition];

    // room on the current page?
    if(p.page && !p.page->full())
        return 0;
    
    // check the quota
    if(page_count == page_quota) {
        // find the biggest partition
        int max = -1;
        for(unsigned i=0; i < partitions.size(); i++) {
            partition_t &p = partitions[i];
            if(!p.file && (max < 0 || p.size > partitions[max].size))
                max = i;
        }

        // create a file
        partition_t &p = partitions[max];
        p.file = create_tmp_file(p.file_name1, "hash-join-left");
        if(!p.file)
            return 1;

        // send the partition to the file
        page_guard_t page;
        for(page = p.page; page->next; page = tuple_page_t::mount(page->next)) {
            if(page->fwrite_full_page(p.file))
                return 1;
            
            page_count--;
        }

        // write the last page, but don't free it
        if(page->fwrite_full_page(p.file))
            return 1;

        page->clear();
        p.page = page.release();
    }
    else {
        // allocate another page
        tuple_page_t *page = tuple_page_t::alloc(_join->left_tuple_size(), malloc);
        page->next = p.page;
        p.page = page;
    }

    // done!
    p.size++;
    return 0;
}

template <class Action>
int hash_join_stage_t::close_file(partition_list_t::iterator it, Action action) {
    tuple_page_t *page = it->page;

    // file partition? (make sure the file gets closed)
    file_guard_t file = it->file;
    if(!page || page->empty())
        return 0;
    if(page->fwrite_full_page(file))
        return 1;

    return action(it);
}

struct hash_join_stage_t::left_action_t {
    size_t _right_tuple_size;
    left_action_t(size_t right_tuple_size)
        : _right_tuple_size(right_tuple_size)
    {
    }
    int operator()(partition_list_t::iterator it) {
        // open a new file for the right side partition
        it->file = create_tmp_file(it->file_name2, "hash-join-right");
        if(!it->file)
            return RESULT_ERROR;

        // resize the page to match right-side tuples
        it->page = tuple_page_t::init(it->page, _right_tuple_size);
        return 0;
    }
};

struct hash_join_stage_t::right_action_t {
    int operator ()(partition_list_t::iterator it) {
        it->file = NULL;
        return 0;
    }
};

stage_t::result_t hash_join_stage_t::process_packet() {
    hash_join_packet_t *packet = (hash_join_packet_t *)_adaptor->get_packet();

    // TODO: release partition resources!
    
    _join = packet->_join;

    // anything worth reading?
    tuple_buffer_t *left_buffer = packet->_left_buffer;
    dispatcher_t::dispatch_packet(packet->_left);
    if(left_buffer->wait_for_input())
        return stage_t::RESULT_STOP;
    
    
    // read in the left relation
    hash_key_t hash_key(_join->key_size());
    extract_key_t<true> extract_left_key(_join);
    tuple_t left;
    while(1) {
        int result = left_buffer->get_tuple(left);
        if(result < 0)
           return stage_t::RESULT_ERROR;

        // eof?
        if(result > 0)
            break;
        
        // use the "lazy mod" approach to reduce the hash range
        int hash_code = hash_key(extract_left_key(left.data));
        int partition = hash_code % partitions.size();
        tuple_page_t* &page = partitions[partition].page;

        // make sure the page isn't full before writing
        if(test_overflow(partition))
            return stage_t::RESULT_ERROR;

        // write to the page
        page->append_init(left);
    }

    // create and fill the in-memory hash table
    size_t page_capacity = tuple_page_t::capacity(4096, _join->left_tuple_size());
    tuple_hash_t table(page_count*page_capacity,
                       hash_key,
                       equal_key_t(_join->key_size()),
                       extract_left_key);
    
    // flush any partitions that went to disk
    left_action_t left_action(_join->right_tuple_size());
    for(partition_list_t::iterator it=partitions.begin(); it != partitions.end(); ++it) {
        tuple_page_t *page = it->page;
        // empty partition?
        if(!page)
            continue;

        // file partition? (make sure the file gets closed)
        if(it->file) {
            if(close_file(it, left_action))
                return RESULT_ERROR;
        }
        
        // build hash table out of in-memory partition
        else {
            while(page) {
                for(tuple_page_t::iterator it=page->begin(); it != page->end(); ++it) 
                    table.insert_equal_noresize(it->data);

                page = (tuple_page_t *)page->next;
            }
        }
    }

    // anything worth reading?
    tuple_buffer_t *right_buffer = packet->_right_buffer;
    dispatcher_t::dispatch_packet(packet->_right);
    if(right_buffer->wait_for_input())
        return stage_t::RESULT_STOP;

    // read in the right relation now
    extract_key_t<false> extract_right_key(_join);
    tuple_t right(NULL, _join->right_tuple_size());
    while(1) {
        int result = right_buffer->get_tuple(right);
        if(result < 0)
           return stage_t::RESULT_ERROR;

        // eof?
        if(result > 0)
            break;
     
        // which partition?
        char *right_key = extract_right_key(right.data);
        int hash_code = hash_key(right_key);
        int partition = hash_code % partitions.size();
        partition_t &p = partitions[partition];

        // empty partition?
        if(p.size == 0)
            continue;

        // add to file partition?
        if(p.file) {
            tuple_page_t *page = p.page;

            // flush to disk?
            if(page->full()) {
                if(page->fwrite_full_page(p.file))
                    return RESULT_ERROR;
                page->clear();
            }

            // add the tuple to the page
            page->append_init(right);
        }

        // check in-memory hash table
        else {
            std::pair<tuple_hash_t::iterator, tuple_hash_t::iterator> range;
            range = table.equal_range(right_key);

            char data[_join->out_tuple_size()];
            tuple_t out(data, sizeof(data));
            for(tuple_hash_t::iterator it = range.first; it != range.second; ++it) {
                left.data = *it;
                _join->join(out, left, right);
                result_t result = _adaptor->output(out);
                if(result)
                    return result;
            }
        }
    }

    // close all the files and release in-memory pages
    table.clear();
    for(partition_list_t::iterator it=partitions.begin(); it != partitions.end(); ++it) {
        // mark the page list for deletion...
        page_list_guard_t page = it->page;
        if(it->file) {
            if(close_file(it, right_action_t()))
                return RESULT_ERROR;
        }
    }

    // handle the file partitions now...
    return RESULT_STOP;
}

#if 0

struct tuple_source {
    size_t _offset, _size;

    // offset is in bytes, size is in ints
    tuple_source(size_t attr_offset, size_t attr_size)
        : _offset(attr_offset), _size(attr_size)
    {
    }

    int gen_key(char *data) {
        int *keyloc = (int *) &data[_offset];
# if defined(__GNUC__) && defined(sparc)
        int res;
        memcpy(&res, keyloc, sizeof(int));
# else
        int res = *(int*)keyloc;
# endif

        while(_size > 1) {
            keyloc++;
            _size--;
            res = res * 2;
            res += *keyloc;
        }
        return res;
    }

};

struct first_pass_tuple_source : public tuple_source {
    tuple_buffer_t *_buffer;
    int *_count;
    FILE **_files;
    tuple_page_t **_pages;
    int _num_partitions;
    first_pass_tuple_source(size_t attr_offset, size_t attr_size,
                            tuple_buffer_t *buffer, int *count,
                            FILE **files, tuple_page_t **pages,
                            int num_partitions)
        : tuple_source(attr_offset, attr_size),
          _buffer(buffer), _count(count),
          _files(files), _pages(pages),
          _num_partitions(num_partitions)
    {
    }
    
    // writes out a page to file
    void flush_page(int partition_num) {
        _pages[partition_num]->fwrite(_files[partition_num]);
        _pages[partition_num]->clear();
    }

    // return false if no entries were written
    bool fill_page(tuple_page_t *page) {
        tuple_t tuple;

        _pages[0] = page;
        while(!_buffer->get_tuple(tuple)) {
            int key = gen_key(tuple.data);
            int partition_num = key % _num_partitions;
            tuple_page_t *page = _pages[partition_num];

            ++*_count;
            
            // copy the tuple into the page
            page->append_init(tuple);
            if(page->full()) {
                if(partition_num == 0)
                    return true;

                flush_page(partition_num);
            }
        }

        // Flush buffer pages to file
        for(int i=1; i < _num_partitions; i++) 
            flush_page(i);

        // all done!
        return !page->empty();
    }

    size_t tuple_size() {
        return _buffer->tuple_size;
    }

    void terminate() {
        _buffer->close_buffer();
    }
};



struct file_tuple_source : public tuple_source {
    FILE *_file;
    size_t _tuple_size;

    file_tuple_source(size_t attr_offset, size_t attr_size,
                      FILE *file, size_t tuple_size)
        : tuple_source(attr_offset, attr_size),
          _file(file), _tuple_size(tuple_size)
    {
    }

    bool fill_page(tuple_page_t *page) {
        return page->fread(_file);
    }

    size_t tuple_size() {
        return _tuple_size;
    }

    void terminate() { }
};

template<class TupleSource>
page_t *hash_join_stage::build_hash(tuple_hash_t &probe_table,
                                          TupleSource source)
{
    page_t *page_list=NULL;
    tuple_page_t *page = tuple_page_t::alloc(source.tuple_size(), malloc);
    while( source.fill_page(page) ) {
        for(tuple_page_t::iterator it = page->begin(); it != page->end(); ++it) {
            char *data = it->data;
            int key = source.gen_key(data);
            probe_table.insert(key, data);
        }

        // next!
        page->next = page_list;
        page_list = page;
        page = tuple_page_t::alloc(source.tuple_size(), malloc);
    }

    // free the memory we used
    // FIXME: dealloc the page list after using the hash table...
    free(page);

    return page_list;
}

template <class TupleSource>
hash_join_packet_t *
hash_join_stage::probe_matches(tuple_hash_t &probe_table,
                               hash_join_packet_t *packet,
                               TupleSource source) {

    bool equality_test = packet->equality_test;
    tuple_page_t *page = tuple_page_t::alloc(source.tuple_size(), malloc);
    bool early_termination = false;
    while( !early_termination && source.fill_page(page) ) {
        for(tuple_page_t::iterator it = page->begin(); it != page->end(); ++it) {
            const tuple_t &right_tup = *it;
            int key = source.gen_key(right_tup.data);
            tuple_hash_t::iterator iter = probe_table.find(key);

            if(iter == probe_table.end() && !equality_test)
                iter = probe_table.begin();
            
            while (iter != probe_table.end()) {
                if(equality_test && (*iter).first != key)
                    continue;
                
                tuple_t left_tup(source.tuple_size(), (*iter).second);
      
                // join two matching tuples
                packet = process_tuple(&left_tup, &right_tup, packet);
                if (!packet) {
                    // early termination by the parent
                    source.terminate();
                    break;
                }
                

                // advance the iterator
                if(equality_test)
                    iter = iter.next_dup();
                else
                    ++iter;
            }
        }

        // next!
        page->clear();
    }

    // done!
    free(page);
    return packet;
}

int hash_join_stage::dequeue()
{
    hash_join_packet_t* packet = (hash_join_packet_t*)queue->remove();

    if (packet == NULL) {
        if (DBG_MSG) fprintf(stderr, "[hash_join_stage::dequeue] NULL packet\n");
        return (-1);
    }  

    TRACE_MONITOR("|%d|P=%d;S=%d;TH=%x", MSG_DEQUEUE_PACKET, packet->unique_id, STAGE_HASH_JOIN, (uint)pthread_self());


    /* check for rebinding directions from the dispatcher */
    if ( packet->bind_cpu != NULL )
        dispatcher_cpu_bind_self( packet->bind_cpu );
  
  
    // wait till our parents allow us to proceed
    if (packet->output_buffer->wait_init() != 0) {
        packet->input_buffer1->close_buffer();
        packet->input_buffer2->close_buffer();

        delete packet;
        return (0);

    } else {
        packet->input_buffer1->init_buffer();
    }
  
    side_queue->insert(packet);


    hash_join_packet_t *temp_packet;
    int left_tuple_size = packet->input_buffer1->tuple_size;
    int right_tuple_size = packet->input_buffer2->tuple_size;
    char fname[512];
  
    // tuple counter
    packet->count_left = 0; packet->count_right = 0; packet->count_out = 0;
  
    // create hash table
    tuple_hash_t probe_table(packet->join_buffer_size
                             / sizeof(tuple_hash_t::hash_bucket));
  
    // open files to dump partitions into
    int num_partitions = packet->num_partitions;
    FILE *partition_files1[num_partitions];
    FILE *partition_files2[num_partitions];
    tuple_page_t* partition_pages[num_partitions];

    // if the left incoming buffer does not return any tuple
    if (!packet->input_buffer1->wait_for_input()) {
        packet->output_buffer->send_eof();

        // notify monitor that packet finished
        TRACE_MONITOR("|%d|P=%d;S=%d;TH=%x|", MSG_PACKET_FINISHED, packet->unique_id, STAGE_HASH_JOIN, (uint)pthread_self());

        delete packet;
        return (0);
    }
  
    // initialize the partitions now (index 0 always invalid!!!)
    for (int i = 1; i < num_partitions; i++) {
        sprintf(fname, "%s/hash%d_file1_part%d", TMP_DIR, packet->hash_join_id, i);
        partition_files1[i] = fopen(fname, "w");

        sprintf(fname, "%s/hash%d_file2_part%d", TMP_DIR, packet->hash_join_id, i);
        partition_files2[i] = fopen(fname, "w");

        // TODO: something besides malloc
        partition_pages[i] = tuple_page_t::alloc(left_tuple_size, malloc);
    }
  

    // partition left side and build partition 0 hash
    first_pass_tuple_source source1(packet->join_attribute_offset1,
                                    packet->join_length,
                                    packet->input_buffer1,
                                    &packet->count_left,
                                    partition_files1, partition_pages,
                                    num_partitions);
    page_t *page_list = build_hash(probe_table, source1);

    TRACE_DEBUG("Hash Join partitioned %u tuples to the left (%d partitions)\n",
                packet->count_left,
                packet->num_partitions);
  


    // reset pages for use with the right side
    for(int i=1; i < num_partitions; i++) 
        tuple_page_t::init(partition_pages[i], right_tuple_size);
  
    packet->input_buffer2->init_buffer();
  
    // remove ourselves from side queue - we are no longer mergable
    side_queue->remove(packet);
  
    // partition right side and join partition 0
    first_pass_tuple_source source2(packet->join_attribute_offset2,
                                    packet->join_length,
                                    packet->input_buffer2,
                                    &packet->count_right,
                                    partition_files2, partition_pages,
                                    num_partitions);
    temp_packet = probe_matches(probe_table, packet, source2);
  
    TRACE_DEBUG("Hash Join partitioned %u tuples to the right (%d partitions)\n",
                packet->count_right, packet->num_partitions);

    // free buffer pages
    while(page_list) {
        page_t *page = page_list;
        page_list = page_list->next;
        free(page);
    }
    
    // close partition files
    for (int i = 1; i < num_partitions; i++) {
        fclose(partition_files1[i]);
        fclose(partition_files2[i]);
        free(partition_pages[i]);
    }
  
    if (!temp_packet) {
        packet->input_buffer2->close_buffer();

        // Ryan: is this the right way to close the output buffer?
        // send result to the waiting (chained) packets
        process_tuple(NULL, NULL, packet);
        
        // notify monitor that packet finished
        TRACE_MONITOR("|%d|P=%d;S=%d;TH=%x|", MSG_PACKET_FINISHED,
                      packet->unique_id, STAGE_HASH_JOIN, (uint)pthread_self());
    
        delete packet;
        return 0;
    }

    packet = temp_packet;

    // join the remaining partitions
    for(int i=1; i < num_partitions; i++) {
        TRACE_DEBUG("Hash-Joining partition %d\n", i);
        
        probe_table.clear();
        
        sprintf(fname, "%s/hash%d_file1_part%d", TMP_DIR,
                packet->hash_join_id, i);
        FILE *file1 = fopen(fname, "r");
        
        sprintf(fname, "%s/hash%d_file2_part%d", TMP_DIR,
                packet->hash_join_id, i);
        FILE *file2 = fopen(fname, "r");
        
        file_tuple_source source1(packet->join_attribute_offset1,
                                  packet->join_length,
                                  partition_files1[i],
                                  left_tuple_size);
        page_list = build_hash(probe_table, source1);
      
        file_tuple_source source2(packet->join_attribute_offset2,
                                  packet->join_length,
                                  partition_files2[i],
                                  right_tuple_size);
        temp_packet = probe_matches(probe_table, packet, source2);
        fclose(file1);
        fclose(file2);

        // free buffer pages
        while(page_list) {
            page_t *page = page_list;
            page_list = page_list->next;
            free(page);
        }
    
        if(!temp_packet)
            break;

        packet = temp_packet;
    }
  
    // send result to the waiting (chained) packets
    process_tuple(NULL, NULL, packet);


  
    TRACE_DEBUG(".: TH%d Hash Join %s: |L|=%d, |R|=%d, |O|=%d\n",
                pthread_self(), packet->packet_string_id, packet->count_left,
                packet->count_right, packet->count_out);
    
    // remove partition files
    for (int i = 1; i < packet->num_partitions; i++) {
        sprintf(fname, "%s/hash%d_file1_part%d", TMP_DIR,
                packet->hash_join_id, i);
        remove(fname);
        sprintf(fname, "%s/hash%d_file2_part%d", TMP_DIR,
                packet->hash_join_id, i);
        remove(fname);
    }

    // notify monitor that packet finished
    TRACE_MONITOR("|%d|P=%d;S=%d;TH=%x|", MSG_PACKET_FINISHED, packet->unique_id,
                  STAGE_HASH_JOIN, (uint)pthread_self());

    delete packet;
    return 0;
}



hash_join_packet_t* hash_join_stage::process_tuple(const tuple_t *left_tup,
                                                   const tuple_t* right_tup,
                                                   hash_join_packet_t* packet)
{
  hash_join_packet_t* cur_packet = packet;
  hash_join_packet_t* prev_packet = NULL;
  hash_join_packet_t* temp_packet = NULL;
  
  if (!left_tup || !right_tup) {
    do {
        cur_packet->output_buffer->send_eof();
      temp_packet = cur_packet;
      cur_packet = (hash_join_packet_t*)cur_packet->next_packet;
      if (temp_packet != packet)
        delete temp_packet;
    } while (cur_packet);
    TRACE_DEBUG("hash_join_stage::process_tuple returned immediately\n");
    return NULL;
  }
  
  bool first_deleted = false;
  
  while (cur_packet) {	
    
      // join two matching tuples
      tuple_t out_tup;
      if (cur_packet->output_buffer->alloc_tuple(out_tup) != 0) {
          if (!prev_packet) {
              first_deleted = true;
              cur_packet = (hash_join_packet_t*)cur_packet->next_packet;
          } else {
              prev_packet->next_packet = cur_packet->next_packet;
              temp_packet = cur_packet;
              cur_packet = (hash_join_packet_t*)cur_packet->next_packet;
	  
              TRACE_MONITOR("|%d|P=%d;S=%d;TH=%x|", MSG_PACKET_FINISHED, temp_packet->unique_id, STAGE_HASH_JOIN, (uint)pthread_self());
	
              delete temp_packet;
          }							
          continue;
      }
      
      cur_packet->join_func->create_output_tuple(out_tup, *left_tup, *right_tup);
      packet->count_out++;
  }
  
  prev_packet = cur_packet;
  cur_packet = (hash_join_packet_t*)cur_packet->next_packet;

  
  if (first_deleted) {
    // check if there is anything left in a chain
    if (packet->next_packet) {
      temp_packet = packet;
      packet = (hash_join_packet_t*)packet->next_packet;

      TRACE_MONITOR("|%d|P=%d;S=%d;TH=%x|", MSG_PACKET_FINISHED, temp_packet->unique_id, STAGE_HASH_JOIN, (uint)pthread_self());

      delete temp_packet;
    } 
    else {
      // we are done with all the chained packets
      return NULL;
    }
  }
  
  return packet;
}
#endif
