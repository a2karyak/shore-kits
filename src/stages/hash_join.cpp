/* -*- mode:C++; c-basic-offset:4 -*- */

/* hash_join_stage.cpp */
/* Implementation of the HASH_JOIN operator */
/* History: 
   3/6/2006: Uses the outtup variable of the output_tup_t, instead of the data.
*/


#include "stages/hash_join.h"

#include <cstring>
#include <algorithm>



const c_str hash_join_packet_t::PACKET_TYPE = "HASH_JOIN";

const c_str hash_join_stage_t::DEFAULT_STAGE_NAME = "HASH_JOIN";



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
    tuple_join_t *_join;

    extract_key_t(tuple_join_t *join)
        : _join(join)
    {
    }
    const char* const operator()(const char * value) const {
        return right? _join->get_right_key(value) : _join->get_left_key(value);
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



void hash_join_stage_t::test_overflow(int partition) {
    partition_t &p = partitions[partition];

    // room on the current page?
    if(p._page && !p._page->full())
        return ;
    
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

        // send the partition to the file
        guard<page> head;
        for(head = p._page; head->next; head=head->next) {
            head->fwrite_full_page(p.file);
            page_count--;
        }

        // write the last page, but don't free it
        head->fwrite_full_page(p.file);
        head->clear();
        p._page = head.release();
    }
    else {
        // allocate another page
        page* pg = page::alloc(_join->right_tuple_size());
        pg->next = p._page;
        p._page = pg;
        page_count++;
    }

    // done!
    p.size++;
}



template <class Action>
void hash_join_stage_t::close_file(partition_list_t::iterator it, Action action) {
    page *p = it->_page;

    // file partition? (make sure the file gets closed)
    file_guard_t file = it->file;
    if(!p || p->empty())
        return;
    
    p->fwrite_full_page(file);
    action(it);
}



struct hash_join_stage_t::right_action_t {
    size_t _left_tuple_size;
    right_action_t(size_t left_tuple_size)
        : _left_tuple_size(left_tuple_size)
    {
    }
    void operator()(partition_list_t::iterator it) {
        // open a new file for the left side partition
        it->file = create_tmp_file(it->file_name2, "hash-join-left");

        // resize the page to match left-side tuples
        it->_page = page::alloc(_left_tuple_size);
    }
};



struct hash_join_stage_t::left_action_t {
    void operator ()(partition_list_t::iterator it) {
        it->file = NULL;
    }
};



void hash_join_stage_t::process_packet() {

    hash_join_packet_t* packet = (hash_join_packet_t *)_adaptor->get_packet();

    // TODO: release partition resources!
    bool outer_join = packet->_outer;
    _join = packet->_join;
    bool distinct = packet->_distinct;

    // anything worth reading?
    tuple_fifo *right_buffer = packet->_right_buffer;
    dispatcher_t::dispatch_packet(packet->_right);


    if(!right_buffer->ensure_read_ready())
        // No right-side tuples... no join tuples.
        return;
    
    
    // read in the right relation
    hash_key_t hash_key(_join->key_size());
    extract_key_t<true> extract_right_key(_join);
    tuple_t right;
    while(1) {

        // eof?
        if(!right_buffer->get_tuple(right))
            break;

        // use the "lazy mod" approach to reduce the hash range
        int hash_code = hash_key(extract_right_key(right.data));
        int partition = hash_code % partitions.size();
        page* &p = partitions[partition]._page;

        // make sure the page isn't full before writing
        test_overflow(partition);

        // write to the page
        p->append_tuple(right);
    }

    // create and fill the in-memory hash table
    size_t page_capacity = page::capacity(get_default_page_size(),
                                          _join->right_tuple_size());
    tuple_hash_t table(page_count*page_capacity,
                       hash_key,
                       equal_key_t(_join->key_size()),
                       extract_right_key);
    
    // flush any partitions that went to disk
    right_action_t right_action(_join->left_tuple_size());
    for(partition_list_t::iterator it=partitions.begin(); it != partitions.end(); ++it) {

        page* p = it->_page;
        if(p == NULL)
            // empty partition
            continue;

        // file partition? (make sure the file gets closed)
        if(it->file) 
            close_file(it, right_action);
        
        // build hash table out of in-memory partition
        else {
            while(p) {
                for(page::iterator it=p->begin(); it != p->end(); ++it) {
                    // Distinguish between DISTINCT join and
                    // non-DISTINCT join, as DB/2 does
                    if(distinct)
                        table.insert_unique_noresize(it->data);
                    else
                        table.insert_equal_noresize(it->data);
                }

                p = p->next;
            }
        }
    }


    // start building left side hash partitions
    tuple_fifo *left_buffer = packet->_left_buffer;
    dispatcher_t::dispatch_packet(packet->_left);
    if(!left_buffer->ensure_read_ready())
        // No left-side tuples... no join tuples.
        return;

    
    // read in the left relation now
    extract_key_t<false> extract_left_key(_join);
    tuple_t left(NULL, _join->left_tuple_size());
    while(1) {

        // eof?
        if(!left_buffer->get_tuple(left))
            break;
     
        // which partition?
        const char* left_key = extract_left_key(left.data);
        int hash_code = hash_key(left_key);
        int partition = hash_code % partitions.size();
        partition_t &p = partitions[partition];

        // empty partition?
        if(p.size == 0)
            continue;

        // add to file partition?
        if(p.file) {
            page* pg = p._page;

            // flush to disk?
            if(pg->full()) {
                pg->fwrite_full_page(p.file);
                pg->clear();
            }

            // add the tuple to the page
            pg->append_tuple(left);
        }

        // check in-memory hash table
        else {
            std::pair<tuple_hash_t::iterator, tuple_hash_t::iterator> range;
            range = table.equal_range(left_key);

            array_guard_t<char> data = new char[_join->out_tuple_size()];
            tuple_t out(data, sizeof(data));
            if(outer_join && range.first == range.second) {
                _join->outer_join(out, left);
                _adaptor->output(out);
            }
            else {
                for(tuple_hash_t::iterator it = range.first; it != range.second; ++it) {
                    right.data = *it;
                    _join->join(out, left, right);
                    _adaptor->output(out);
                }
            }
        }
    }

    // close all the files and release in-memory pages
    table.clear();
    for(partition_list_t::iterator it=partitions.begin(); it != partitions.end(); ++it) {
        // delete the page list
        for(guard<page> pg = it->_page; pg; pg = pg->next);
        if(it->file) 
            close_file(it, left_action_t());
    }

    // TODO: handle the file partitions now...
}
