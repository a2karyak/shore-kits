/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   bptree.h
 *
 *  @brief:  Declaration of an in-memory B+ tree with latches class
 *
 *  @author: Ippokratis Pandis (ipandis)
 *
 *  @history
 *  10/17/07; When there is a split the node is not released
 *  10/16/07: Adding Reader-Writer Locks to the nodes and shared variables
 */

#ifndef __BPLUSTREE_H
#define __BPLUSTREE_H

// This is required for glibc to define std::posix_memalign
#if !defined (_XOPEN_SOURCE) || (_XOPEN_SOURCE < 600)
#define _XOPEN_SOURCE 600
#endif
#include <stdlib.h>

#include <assert.h>
#include <boost/static_assert.hpp>
#include <boost/pool/object_pool.hpp>

// for critical_section_t
#include "util/sync.h"

// DEBUG
#include <iostream>
//using std::cout;
//using std::endl;

#ifdef DEBUG_ASSERT
#    warning "DEBUG_ASSERT overloaded"
#else
#    ifdef DEBUG
#        define DEBUG_ASSERT(expr) assert(expr)
#    else
#        define DEBUG_ASSERT(expr)
#    endif
#endif

template <typename KEY, typename VALUE, 
          unsigned INODE_ENTRIES, unsigned LEAF_ENTRIES,
          unsigned INODE_PADDING = 0, unsigned LEAF_PADDING = 0,
          unsigned NODE_ALIGNMENT = 64>
class BPlusTree
{
public:
    // INODE_ENTRIES must be greater than two to make the split of
    // two inner nodes sensible.
    BOOST_STATIC_ASSERT(INODE_ENTRIES>2);
    
    // Leaf nodes must be able to hold at least one element
    BOOST_STATIC_ASSERT(LEAF_ENTRIES>0);
    
    // Builds a new empty tree.
    BPlusTree()
        : depth(0)
    {
        // DEBUG
        // cout << "sizeof(LeafNode)==" << sizeof(LeafNode) << endl;
        // cout << "sizeof(InnerNode)==" << sizeof(InnerNode) << endl;

        // Initialization of mutexes
        pthread_mutex_init(&innerPool_mutex, NULL);
        pthread_mutex_init(&leafPool_mutex, NULL);
        pthread_rwlock_init(&depth_rwlock, NULL);

        // Initialization of the root, should be done after mutex init
        root = new_leaf_node();
    }

    ~BPlusTree() 
    {
        // Memory deallocation is done automatically
        // when innerPool and leafPool are destroyed.

        // Destroes the mutexes
        pthread_mutex_destroy(&innerPool_mutex);
        pthread_mutex_destroy(&leafPool_mutex);
        pthread_rwlock_destroy(&depth_rwlock);
    }
    
    // Inserts a pair (key, value). If there is a previous pair with
    // the same key, the old value is overwritten with the new one.
    void insert(KEY key, VALUE value) 
    {
        InsertionResult result;
        bool was_split;

        pthread_rwlock_rdlock(&depth_rwlock);        
        if ( depth == 0 ) {
            // The root is a leaf node
            DEBUG_ASSERT( *reinterpret_cast<NodeType*>(root) == NODE_LEAF );
            was_split = leaf_insert(reinterpret_cast<LeafNode*>(root), 
                                    key, value, &result);
        } 
        else {
            // The root is an inner node
            DEBUG_ASSERT( *reinterpret_cast<NodeType*>(root) == NODE_INNER );
            was_split = inner_insert(reinterpret_cast<InnerNode*>(root), 
                                     depth, key, value, &result);
        }
        pthread_rwlock_unlock(&depth_rwlock);        

        if (was_split) {
            // The old root was splitted in two parts.
            // We have to create a new root pointing to them
            pthread_rwlock_wrlock(&depth_rwlock);
            depth++;
            cout << "Depth = " << depth << "\n";

            // Create the new root node
            root = new_inner_node();
            InnerNode* rootProxy = reinterpret_cast<InnerNode*>(root);
            rootProxy->num_keys = 1;
            rootProxy->keys[0] = result.key;
            rootProxy->children[0] = result.left;
            rootProxy->children[1] = result.right;

            // The new root InnerNode was created and returned locked
            // it should be released now
            pthread_rwlock_unlock(&depth_rwlock);
        }
    }
    

    // Looks for the given key. If it is not found, it returns false,
    // if it is found, it returns true and copies the associated value
    // unless the pointer is null.
    bool find(const KEY& key, VALUE* value = 0) const 
    {
        InnerNode* inner;
        register void* node = root;
        register unsigned d = depth;
        register unsigned index;
    
        // Drills down the InnerNodes
        while ( d-- != 0 ) {
            assert(node);
            
            inner = reinterpret_cast<InnerNode*>(node);

            // Read locks the node
            pthread_rwlock_rdlock(&(inner->in_rwlock));

            DEBUG_ASSERT( inner->type == NODE_INNER );
            index = inner_position_for(key, inner->keys, inner->num_keys);
            node = inner->children[index];

            // Unlocks the node
            pthread_rwlock_unlock(&(inner->in_rwlock));
        }
        
        // Reached the correct leaf node
        assert(node);
        LeafNode* leaf= reinterpret_cast<LeafNode*>(node);
        DEBUG_ASSERT( leaf->type == NODE_LEAF );

        pthread_rwlock_rdlock(&(leaf->leaf_rwlock));
        index= leaf_position_for(key, leaf->keys, leaf->num_keys);
        
        if ( leaf->keys[index] == key ) {
            // Entry found, copies the value and returns true
            if ( value != 0 ) {
                *value= leaf->values[index];
            }
            pthread_rwlock_unlock(&(leaf->leaf_rwlock));
            return true;
        } 
        else {
            // No entry with that key found, returns false
            pthread_rwlock_unlock(&(leaf->leaf_rwlock));
            return false;
        }
    }
    
    // Returns the size of an inner node
    // It is useful when optimizing performance with cache alignment.
    unsigned sizeof_inner_node() const 
    {
        return sizeof(InnerNode);
    }

    // Returns the size of a leaf node.
    // It is useful when optimizing performance with cache alignment.
    unsigned sizeof_leaf_node() const 
    {
        return sizeof(LeafNode);
    }        


    // Overrides operator<< (for ostreams)
    template <typename AKEY, typename AVALUE, 
              unsigned AINODE_ENTRIES, unsigned ALEAF_ENTRIES,
              unsigned AINODE_PADDING, unsigned ALEAF_PADDING,
              unsigned ANODE_ALIGNMENT>
    friend std::ostream& operator<<(std::ostream& out, 
                                    BPlusTree<AKEY, AVALUE, AINODE_ENTRIES, ALEAF_ENTRIES,
                                    AINODE_PADDING, ALEAF_PADDING, ANODE_ALIGNMENT>& bpt)
    {
        out << "bpt: " << sizeof(bpt) 
            << " IN_SZ= " << bpt.sizeof_inner_node()
            << " LE_SZ= " << bpt.sizeof_leaf_node() << "\n";
        
        return (out);
    }


private:
    // Used when debugging
    enum NodeType {NODE_INNER=0x1111, NODE_LEAF=0x2323};
    
    // Leaf nodes store pairs of keys and values.
    struct LeafNode {
#ifdef DEBUG
        // Debug mode constructor
        LeafNode() : type(NODE_LEAF), num_keys(0) {
            // rwlock initialization
            pthread_rwlock_init(&leaf_rwlock, NULL);
        }
        const NodeType type;
#else
        LeafNode() : num_keys(0) { 
            // rwlock initialization
            pthread_rwlock_init(&leaf_rwlock, NULL);
        }
#endif
        // Destructor
        ~LeafNode() {
            // destroy the rwlock
            pthread_rwlock_destroy(&leaf_rwlock);
        }

        // Reader-Writer Lock
        pthread_rwlock_t leaf_rwlock;

        unsigned num_keys;
        KEY      keys[LEAF_ENTRIES];
        VALUE    values[LEAF_ENTRIES];
        unsigned char _pad[LEAF_PADDING];
    };

    /** @struct: Innernode
     *  @desc:   A structure for the internal nodes of the BPTree
     */

    struct InnerNode {
#ifdef DEBUG
        // Debug mode constructor
        InnerNode() : type(NODE_INNER), num_keys(0) {
            // rwlock initialization
            pthread_rwlock_init(&in_rwlock, NULL);
        }
        const NodeType type;
#else
        // Constructor
        InnerNode() : num_keys(0) {
            // rwlock initialization
            pthread_rwlock_init(&in_rwlock, NULL);
        }
#endif
        // Destructor
        ~InnerNode() {
            // destroy the rwlock
            pthread_rwlock_destroy(&in_rwlock);
        }

        // Reader-Writer Lock
        pthread_rwlock_t in_rwlock;

        unsigned num_keys;
        KEY      keys[INODE_ENTRIES];
        void*    children[INODE_ENTRIES+1];
        unsigned char _pad[INODE_PADDING];
    };

    // Custom allocator that returns aligned blocks of memory
    template <unsigned ALIGNMENT>
    struct AlignedMemoryAllocator {
        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;
        
        static char* malloc(const size_type bytes) {
            void* result;
            if( posix_memalign(&result, ALIGNMENT, bytes) != 0 ) {
                result= 0;
            }
            
            // Alternative: result= std::malloc(bytes);
            return reinterpret_cast<char*>(result);
        }

        static void free(char* const block) { 
            std::free(block); 
        }
    };


    /** @fn:   new_leaf_node
     *  @desc: Returns a pointer to a fresh leaf node.
     *
     *  @note: Uses the leafPool for this. Made thread_safe.
     */

    LeafNode* new_leaf_node() {
        LeafNode* result;

        // regulate the access to the leafPool
        pthread_mutex_lock(&leafPool_mutex);

        //result= new LeafNode();
        result = leafPool.construct();

        // release leafPool lock
        pthread_mutex_unlock(&leafPool_mutex);

        //cout << "New LeafNode at " << result << endl;
        return result;
    }
    
    // Frees a leaf node previously allocated with new_leaf_node()
    void delete_leaf_node(LeafNode* node) {
        DEBUG_ASSERT( node->type == NODE_LEAF );

        assert (0); // (ip): Not implemented yet!

        //cout << "Deleting LeafNode at " << node << endl;
        // Alternatively: delete node;
        leafPool.destroy(node);
    }


    /** @fn:   new_inner_node
     *  @desc: Returns a pointer to a fresh inner node, locked for writing
     *
     *  @note: Uses the innerNodePool for this. Made thread_safe.
     */

    InnerNode* new_inner_node() {
        InnerNode* result;

        // regulate the access to the innerPool
        pthread_mutex_lock(&innerPool_mutex);

        // Alternatively: result= new InnerNode();
        result = innerPool.construct();

        // release innerPool lock
        pthread_mutex_unlock(&innerPool_mutex);

        //cout << "New InnerNode at " << result << endl;
        return result;
    }

    // Frees an inner node previously allocated with new_inner_node()
    void delete_inner_node(InnerNode* node) {
        DEBUG_ASSERT( node->type == NODE_INNER );

        assert (0); // (ip): Not implemented yet!

        //cout << "Deleting InnerNode at " << node << endl;
        // Alternatively: delete node;
        innerPool.destroy(node);
    }

        
    /** @struct: InsertionResult
     *  @desc:   Data type returned by the private insertion methods.
     */

    struct InsertionResult {
        KEY key;
        void* left;
        void* right;
    };


    // Returns the position where 'key' should be inserted in a leaf node
    // that has the given keys.
    inline static unsigned leaf_position_for(const KEY& key, 
                                             const KEY* keys,
                                             unsigned num_keys) 
    {
        // Simple linear search. Faster for small values of N or M
        unsigned k = 0;
        
        while((k < num_keys) && (keys[k] < key)) {
            ++k;
        }
        return k;
        
        /*
        // Binary search. It is faster when N or M is > 100,
        // but the cost of the division renders it useless
        // for smaller values of N or M.
        XXX--- needs to be re-checked because the linear search
        has changed since the last update to the following ---XXX
        int left= -1, right= num_keys, middle;
        while( right -left > 1 ) {
        middle= (left+right)/2;
        if( keys[middle] < key) {
        left= middle;
        } else {
        right= middle;
        }
        }
        //assert( right == k );
        return unsigned(right);
        */
    }

    // Returns the position where 'key' should be inserted in an inner node
    // that has the given keys.
    inline static unsigned inner_position_for(const KEY& key, 
                                              const KEY* keys,
                                              unsigned num_keys) 
    {
      // Simple linear search. Faster for small values of N or M
        unsigned k = 0;
        while((k < num_keys) && ((keys[k]<key) || (keys[k]==key))) {
            ++k;
        }
        return k;

        // Binary search is faster when N or M is > 100,
        // but the cost of the division renders it useless
        // for smaller values of N or M.
    }


    /** fn:   leaf_insert 
     *  desc: Inserts an entry to a leaf node. If there not enough room
     *  it causes a leaf node split. The thread that inserts something an
     *  entry to a leaf node acquires a lock for the whole node.
     */

    bool leaf_insert(LeafNode* node, KEY& key,
                     VALUE& value, InsertionResult* result) 
    {
        assert(node);

        // Write-Locks the leaf node before insert
        pthread_rwlock_wrlock(&(node->leaf_rwlock));

        DEBUG_ASSERT(node->type == NODE_LEAF);
        assert(node->num_keys <= LEAF_ENTRIES);
        bool was_split= false;
        
        // Simple linear search
        unsigned i = leaf_position_for(key, node->keys, node->num_keys);

        if ( node->num_keys == LEAF_ENTRIES ) {
            // The node was full. We must split it
            unsigned treshold = (LEAF_ENTRIES+1)/2;
            LeafNode* new_sibling = new_leaf_node();
            new_sibling->num_keys = node->num_keys -treshold;
        
            for(unsigned j=0; j < new_sibling->num_keys; ++j) {
                new_sibling->keys[j]   = node->keys[treshold+j];
                new_sibling->values[j] = node->values[treshold+j];
            }
            
            node->num_keys= treshold;
        
            if ( i < treshold ) {
                // Inserted element goes to left sibling
                leaf_insert_nonfull(node, key, value, i);
            } 
            else {
                // Inserted element goes to right sibling
                leaf_insert_nonfull(new_sibling, key, value, i-treshold);
            }
            
            // Notify the parent about the split
            was_split= true;
            result->key= new_sibling->keys[0];
            result->left= node;
            result->right= new_sibling;
        } 
        else {
            // The node was not full
            leaf_insert_nonfull(node, key, value, i);
        }

        // Insertion completed, unlocks the node
        pthread_rwlock_unlock(&(node->leaf_rwlock));
        
        return was_split;        
    }
    

    /** fn:   leaf_insert_nonfull 
     *  desc: Inserts an entry to a non-full leaf node. 
     *  note: The node is already locked by the thread that does the
     *  insertion. This operation should never fail.
     */

    void leaf_insert_nonfull(LeafNode* node, KEY& key, 
                             VALUE& value, unsigned index)
    {
        DEBUG_ASSERT( node->type == NODE_LEAF );
        assert( node->num_keys < LEAF_ENTRIES );
        assert( index <= LEAF_ENTRIES );
        assert( index <= node->num_keys );
        
        if ( (index < LEAF_ENTRIES) && (node->keys[index] == key) ) {
            // We are inserting a duplicate value.
            // Simply overwrite (update) the old one
            node->values[index] = value;
        } 
        else {
            // The key we are inserting is unique

            // We need to create a slot for the new entry
            // So, we are shifting all the entries a slot right
            for (unsigned i=node->num_keys; i > index; --i) {
                node->keys[i] = node->keys[i-1];
                node->values[i] = node->values[i-1];
            }
        
            // Now we can go ahead and do the insertion
            node->keys[index]= key;
            node->values[index]= value;
            node->num_keys++;
        }
    }


    /** fn:   inner_insert 
     *  desc: Inserts an entry to an inner node. If there not enough room
     *  it causes an inner node split. 
     */

    bool inner_insert(InnerNode* node, unsigned depth, 
                      KEY& key, VALUE& value, 
                      InsertionResult* result)
    {
        assert(node);

        // Locks the node before insert
        pthread_rwlock_wrlock(&(node->in_rwlock));

        DEBUG_ASSERT( node->type == NODE_INNER );
        assert( depth != 0 );
        
        // Early split if node is full.
        // This is not the canonical algorithm for B+ trees,
        // but it is simpler and does not break the definition.
        bool was_split= false;
        
        if ( node->num_keys == INODE_ENTRIES ) {
            // Split
            unsigned treshold= (INODE_ENTRIES+1)/2;
            InnerNode* new_sibling = new_inner_node();
            
            new_sibling->num_keys = node->num_keys - treshold;
            
            for (unsigned i=0; i < new_sibling->num_keys; ++i) {
                new_sibling->keys[i] = node->keys[treshold+i];
                new_sibling->children[i] = node->children[treshold+i];
            }
            
            new_sibling->children[new_sibling->num_keys] = 
                node->children[node->num_keys];
            node->num_keys = treshold-1;
        
            // Set up the return variable
            was_split = true;
            result->key = node->keys[treshold-1];
            result->left = node;
            result->right = new_sibling;
    
            // Now insert in the appropriate sibling
            if ( key < result->key ) {
                inner_insert_nonfull(node, depth, key, value);
            } 
            else {
                inner_insert_nonfull(new_sibling, depth, key, value);
            }
        } 
        else {
            // No split
            inner_insert_nonfull(node, depth, key, value);
        }

        // Insertion completed, unlocks the node
        pthread_rwlock_unlock(&(node->in_rwlock));

        return was_split;
    }

    
    // Insert node to a non-full node
    void inner_insert_nonfull(InnerNode* node, unsigned depth, 
                              KEY& key, VALUE& value)
    {
        DEBUG_ASSERT( node->type == NODE_INNER );
        assert( node->num_keys < INODE_ENTRIES );
        assert( depth != 0 );
        
        // Simple linear search
        unsigned index= inner_position_for(key, 
                                           node->keys,
                                           node->num_keys);
        InsertionResult result;
        bool was_split;
        
        if ( depth-1 == 0 ) {
            // The children are leaf nodes
            for (unsigned kk=0; kk < node->num_keys+1; ++kk) {
                DEBUG_ASSERT( *reinterpret_cast<NodeType*>
                              (node->children[kk]) == NODE_LEAF );
            }
            
            was_split= leaf_insert(reinterpret_cast<LeafNode*>
                                   (node->children[index]), 
                                   key, value, &result);
        } 
        else {
            // The children are inner nodes
            for (unsigned kk=0; kk < node->num_keys+1; ++kk) {
                DEBUG_ASSERT( *reinterpret_cast<NodeType*>
                              (node->children[kk]) == NODE_INNER );
            }
            
            InnerNode* child = reinterpret_cast<InnerNode*>(node->children[index]);
            was_split= inner_insert(child, depth-1, key, value, &result);
        }
        
        if ( was_split ) {
            if ( index == node->num_keys ) {
                // Insertion at the rightmost key
                node->keys[index]= result.key;
                node->children[index]= result.left;
                node->children[index+1]= result.right;
                node->num_keys++;
            } 
            else {
                // Insertion not at the rightmost key
                node->children[node->num_keys+1] = node->children[node->num_keys];
                
                for (unsigned i=node->num_keys; i!=index; --i) {
                    node->children[i] = node->children[i-1];
                    node->keys[i] = node->keys[i-1];
                }
            
                node->children[index] = result.left;
                node->children[index+1] = result.right;
                node->keys[index] = result.key;
                node->num_keys++;
            }
        } // else the current node is not affected
    }

    typedef AlignedMemoryAllocator<NODE_ALIGNMENT> AlignedAllocator;
    
    // Node memory allocators. IMPORTANT NOTE: they must be declared
    // before the root to make sure that they are properly initialised
    // before being used to allocate any node.
    pthread_mutex_t innerPool_mutex;
    boost::object_pool<InnerNode, AlignedAllocator> innerPool;
    pthread_mutex_t leafPool_mutex;
    boost::object_pool<LeafNode, AlignedAllocator>  leafPool;
    
    // Depth of the tree. A tree of depth 0 only has a leaf node.
    pthread_rwlock_t depth_rwlock;
    unsigned depth;

    // Pointer to the root node. It may be a leaf or an inner node, but
    // it is never null.
    void*    root;

}; // EOF: BPlusTree


#endif // __BPLUSTREE_H

