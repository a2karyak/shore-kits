
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

template <bool right>
struct hash_join_stage_t::extract_key_t {
    mutable tuple_join_t *_join;
    mutable array_guard_t<char> _k1;
    mutable array_guard_t<char> _k2;
    mutable tuple_t _tuple;

    extract_key_t(tuple_join_t *join)
        : _join(join),
          _k1(new char[_join->key_size()]),
          _k2(new char[_join->key_size()]),
          _tuple(NULL, right? _join->right_tuple_size() : _join->left_tuple_size())
    {
    }
    extract_key_t(const extract_key_t &other)
        : _join(other._join),
          _k1(new char[_join->key_size()]),
          _k2(new char[_join->key_size()]),
          _tuple(NULL, right? _join->right_tuple_size() : _join->left_tuple_size())
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
        if(right)
            _join->get_right_key(_k1, _tuple);
        else
            _join->get_left_key(_k1, _tuple);
        
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
        p.file = create_tmp_file(p.file_name1, "hash-join-right");
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
        tuple_page_t *page = tuple_page_t::alloc(_join->right_tuple_size(), malloc);
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

struct hash_join_stage_t::right_action_t {
    size_t _left_tuple_size;
    right_action_t(size_t left_tuple_size)
        : _left_tuple_size(left_tuple_size)
    {
    }
    int operator()(partition_list_t::iterator it) {
        // open a new file for the left side partition
        it->file = create_tmp_file(it->file_name2, "hash-join-left");
        if(!it->file)
            return RESULT_ERROR;

        // resize the page to match left-side tuples
        it->page = tuple_page_t::init(it->page, _left_tuple_size);
        return 0;
    }
};

struct hash_join_stage_t::left_action_t {
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
    tuple_buffer_t *right_buffer = packet->_right_buffer;
    dispatcher_t::dispatch_packet(packet->_right);
    // TODO: handle error condition properly
    if(right_buffer->wait_for_input())
        return stage_t::RESULT_STOP;
    
    
    // read in the right relation
    hash_key_t hash_key(_join->key_size());
    extract_key_t<true> extract_right_key(_join);
    tuple_t right;
    while(1) {
        int result = right_buffer->get_tuple(right);
        if(result < 0)
           return stage_t::RESULT_ERROR;

        // eof?
        if(result > 0)
            break;
        
        // use the "lazy mod" approach to reduce the hash range
        int hash_code = hash_key(extract_right_key(right.data));
        int partition = hash_code % partitions.size();
        tuple_page_t* &page = partitions[partition].page;

        // make sure the page isn't full before writing
        if(test_overflow(partition))
            return stage_t::RESULT_ERROR;

        // write to the page
        page->append_init(right);
    }

    // create and fill the in-memory hash table
    size_t page_capacity = tuple_page_t::capacity(4096, _join->right_tuple_size());
    tuple_hash_t table(page_count*page_capacity,
                       hash_key,
                       equal_key_t(_join->key_size()),
                       extract_right_key);
    
    // flush any partitions that went to disk
    right_action_t right_action(_join->left_tuple_size());
    for(partition_list_t::iterator it=partitions.begin(); it != partitions.end(); ++it) {
        tuple_page_t *page = it->page;
        // empty partition?
        if(!page)
            continue;

        // file partition? (make sure the file gets closed)
        if(it->file) {
            if(close_file(it, right_action))
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
    tuple_buffer_t *left_buffer = packet->_left_buffer;
    dispatcher_t::dispatch_packet(packet->_left);
    if(left_buffer->wait_for_input())
        return stage_t::RESULT_STOP;

    // read in the left relation now
    extract_key_t<false> extract_left_key(_join);
    tuple_t left(NULL, _join->left_tuple_size());
    while(1) {
        int result = left_buffer->get_tuple(left);
        if(result < 0)
           return stage_t::RESULT_ERROR;

        // eof?
        if(result > 0)
            break;
     
        // which partition?
        char *left_key = extract_left_key(left.data);
        int hash_code = hash_key(left_key);
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
            page->append_init(left);
        }

        // check in-memory hash table
        else {
            std::pair<tuple_hash_t::iterator, tuple_hash_t::iterator> range;
            range = table.equal_range(left_key);

            char data[_join->out_tuple_size()];
            tuple_t out(data, sizeof(data));
            for(tuple_hash_t::iterator it = range.first; it != range.second; ++it) {
                right.data = *it;
                _join->join(out, right, left);
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
            if(close_file(it, left_action_t()))
                return RESULT_ERROR;
        }
    }

    // handle the file partitions now...
    return RESULT_STOP;
}
