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

#include "util/sync.h"
#include "util/trace.h"

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


    /** @fn:   Constructor
     *  @desc: Builds a new empty tree.
     */

    BPlusTree()
        : depth(0)
    {
        // Initialization of mutexes
        pthread_mutex_init(&innerPool_mutex, NULL);
        pthread_mutex_init(&leafPool_mutex, NULL);
        pthread_mutex_init(&depth_mutex, NULL);

        // Initialization of the root, should be done after mutex init
        root = new_leaf_node();
    }


    /** @fn:   Destructor 
     *  @desc: Destroys the tree.
     */

    ~BPlusTree() 
    {
        // Memory deallocation is done automatically
        // when innerPool and leafPool are destroyed.

        // Destroes the mutexes
        pthread_mutex_destroy(&innerPool_mutex);
        pthread_mutex_destroy(&leafPool_mutex);
        pthread_mutex_destroy(&depth_mutex);
    }


    /** @fn:   insert
     *  @desc: Inserts a pair (key, value). If there is a previous pair 
     *  with the same key, the old value is overwritten with the new 
     *  one (updated). The root is locked at any case and it will be
     *  released later on the drill down
     */

    void insert(KEY key, VALUE value) 
    {
        InsertionResult result;
        bool was_split = false;
        InnerNode* rootProxy = NULL;
        LeafNode* rootLeafProxy = NULL;
        register unsigned oldDepth = depth;

        if ( oldDepth == 0 ) {
            // The root is a leaf node 
            rootLeafProxy = reinterpret_cast<LeafNode*>(root);

            TRACE( TRACE_DEBUG, "root leaf write-lock (%x)\n", rootLeafProxy);
            pthread_rwlock_wrlock(&(rootLeafProxy->leaf_rwlock));

            was_split = leaf_insert(rootLeafProxy, NULL, key, value, &result);
        } 
        else {
            // The root is an inner node
            rootProxy = reinterpret_cast<InnerNode*>(root);

            TRACE( TRACE_DEBUG, "root inner write-lock (%x)\n", rootProxy);
            pthread_rwlock_wrlock(&(rootProxy->in_rwlock));

            was_split = inner_insert(rootProxy, NULL,
                                     depth, key, value, &result);
        }

        if (was_split) {
            // The old root was splitted in two parts.
            // We have to create a new root pointing to them
            // Once we are done we should unlock the old root.
            pthread_mutex_lock(&depth_mutex);

            // Create the new root node
            root = new_inner_node(true);
            InnerNode* newRootProxy = reinterpret_cast<InnerNode*>(root);
            newRootProxy->num_keys = 1;
            newRootProxy->keys[0] = result.key;
            newRootProxy->children[0] = result.left;
            newRootProxy->children[1] = result.right;

            TRACE( TRACE_DEBUG, "new root unlock (%x)\n", newRootProxy);
            pthread_rwlock_unlock(&(newRootProxy->in_rwlock));
            
            if (oldDepth == 0) {
                // the splitted root was a leaf
                TRACE( TRACE_DEBUG, "root leaf unlock (%x)\n", rootLeafProxy);
                pthread_rwlock_unlock(&(rootLeafProxy->leaf_rwlock));
            }
            else {
                // the splitted root was a inner node
                TRACE( TRACE_DEBUG, "root inner unlock (%x)\n", rootProxy);
                pthread_rwlock_unlock(&(rootProxy->in_rwlock));
            }

            depth++;
            TRACE( TRACE_ALWAYS, "Depth = %d\n", depth);

            pthread_mutex_unlock(&depth_mutex);
        }
    }
    

    // Looks for the given key. If it is not found, it returns false,
    // if it is found, it returns true and copies the associated value
    // unless the pointer is null.
    bool find(const KEY& key, VALUE* value = 0) const 
    {
        InnerNode* inner;
        register void* node = root;
        unsigned initDepth = depth;
        register unsigned d = initDepth;
        register unsigned index;
    
        // Drills down the InnerNodes
        while ( d-- != 0 ) {
            assert(node);
            
            inner = reinterpret_cast<InnerNode*>(node);

            // Read locks the node
            TRACE( TRACE_DEBUG, "inner read (%x)\n", node);
            pthread_rwlock_rdlock(&(inner->in_rwlock));

            index = inner_position_for(key, inner->keys, inner->num_keys);
            node = inner->children[index];

            // Unlocks the node
            TRACE( TRACE_DEBUG, "inner unlock (%x)\n", node);
            pthread_rwlock_unlock(&(inner->in_rwlock));
        }
       
        if (initDepth != depth) {
            // The initial depth value has changed will going down the tree
            // It should restart the search
            TRACE( TRACE_ALWAYS, "Depth mismatch. Old (%d) Current (%d)\n", 
                   initDepth, depth);
            return (find(key, value));
        }

        
        // Reached the correct leaf node
        assert(node);

        // Searches within the leaf node
        LeafNode* leaf= reinterpret_cast<LeafNode*>(node);

        TRACE( TRACE_DEBUG, "leaf read (%x)\n", node);
        pthread_rwlock_rdlock(&(leaf->leaf_rwlock));

        index= leaf_position_for(key, leaf->keys, leaf->num_keys);
        
        if ( leaf->keys[index] == key ) {
            // Entry found, copies the value and returns true
            if ( value != 0 ) {
                *value= leaf->values[index];
            }

            TRACE( TRACE_DEBUG, "leaf unlock (%x)\n", node);
            pthread_rwlock_unlock(&(leaf->leaf_rwlock));

            return true;
        } 
        else {
            // No entry with that key found, returns false
            TRACE( TRACE_DEBUG, "leaf unlock (%x)\n", node);
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
        LeafNode() : num_keys(0) { 
            // rwlock initialization
            pthread_rwlock_init(&leaf_rwlock, NULL);
        }

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
        // Constructor
        InnerNode() : num_keys(0) {
            // rwlock initialization
            pthread_rwlock_init(&in_rwlock, NULL);
        }

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
            
            return (reinterpret_cast<char*>(result));
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

        TRACE( TRACE_DEBUG, "New LeafNode at %x\n", result);
        return result;
    }
    
    // Frees a leaf node previously allocated with new_leaf_node()
    void delete_leaf_node(LeafNode* node) {

        assert (0); // (ip): Not implemented yet!

        //cout << "Deleting LeafNode at " << node << endl;
        leafPool.destroy(node);
    }


    /** @fn:   new_inner_node
     *  @desc: Returns a pointer to a fresh inner node, locked for writing
     *
     *  @note: Uses the innerNodePool for this. Made thread_safe.
     */

    InnerNode* new_inner_node(bool bLocked = false) 
    {
        InnerNode* result;

        // regulate the access to the innerPool
        pthread_mutex_lock(&innerPool_mutex);

        result = innerPool.construct();

        if (bLocked) {
            TRACE( TRACE_DEBUG, "new inner write-lock (%x)\n", result);
            pthread_rwlock_wrlock(&(result->in_rwlock));
        }
            

        // release innerPool lock
        pthread_mutex_unlock(&innerPool_mutex);

        TRACE( TRACE_DEBUG, "New InnerNode at %x\n", result);
        return result;
    }

    // Frees an inner node previously allocated with new_inner_node()
    void delete_inner_node(InnerNode* node) {

        assert (0); // (ip): Not implemented yet!

        //cout << "Deleting InnerNode at " << node << endl;
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
     *  it causes a leaf node split. The thread that inserts an
     *  entry to a leaf node acquires a lock for the whole node.
     *  If no split is to be done the lock to the parent node is released.
     */

    bool leaf_insert(LeafNode* node, InnerNode* parentNode,
                     KEY& key, VALUE& value, 
                     InsertionResult* result) 
    {
        assert(node);

        // Write-Locks the leaf node before insert, unless there is no
        // parentNode. If there is no parentNode it means that the 
        // current node is a leaf/root node and it is already locked
        // so there is no need for additional locking

        if (parentNode) {
            TRACE( TRACE_DEBUG, "leaf write (%x)\n", node);
            pthread_rwlock_wrlock(&(node->leaf_rwlock));
        }

        assert(node->num_keys <= LEAF_ENTRIES);
        bool was_split= false;
        
        // Simple linear search
        unsigned i = leaf_position_for(key, node->keys, node->num_keys);

        // Check if Leaf node is full
        if ( node->num_keys == LEAF_ENTRIES ) {
            // The node was full. We must split it
            unsigned treshold = (LEAF_ENTRIES+1)/2;

            // Create a new sibling node and transfers theshold entries
            LeafNode* new_sibling = new_leaf_node();
            new_sibling->num_keys = node->num_keys - treshold;
        
            for(unsigned j=0; j < new_sibling->num_keys; ++j) {
                new_sibling->keys[j]   = node->keys[treshold+j];
                new_sibling->values[j] = node->values[treshold+j];
            }
            
            node->num_keys= treshold;
        
            if (i<treshold) {
                // Inserted element goes to left sibling
                leaf_insert_nonfull(node, key, value, i);
            } 
            else {
                // Inserted element goes to right sibling
                leaf_insert_nonfull(new_sibling, key, value, i-treshold);
            }
            
            // Notify the parent about the split
            was_split = true;
            result->key = new_sibling->keys[0];
            result->left = node;
            result->right = new_sibling;
        } 
        else {            
            if (parentNode) {
                TRACE( TRACE_DEBUG, "releasing dad (%x)\n", parentNode); 
                TRACE( TRACE_DEBUG, "unlock (%x)\n", parentNode);
                pthread_rwlock_unlock(&(parentNode->in_rwlock));
            }
                
            // The node was not full
            leaf_insert_nonfull(node, key, value, i);
        }

        // Insertion completed, unlocks the node
        TRACE( TRACE_DEBUG, "leaf unlock (%x)\n", node);
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
        assert(node->num_keys<LEAF_ENTRIES);
        assert(index<=LEAF_ENTRIES);
        assert(index<=node->num_keys);
        
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

    bool inner_insert(InnerNode* node, InnerNode* parentNode, unsigned depth, 
                      KEY& key, VALUE& value, 
                      InsertionResult* result)
    {
        assert(node);

        // Locks the node before insert
        TRACE( TRACE_DEBUG, "inner writelock (%x)\n", node);        
        pthread_rwlock_wrlock(&(node->in_rwlock));

        // Since we are inserting to an inner node the depth > 0
        assert(depth>0);
        
        // Early split if node is full.
        // This is not the canonical algorithm for B+ trees,
        // but it is simpler and does not break the definition.
        bool was_split = false;
        
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
            // No split, can release parent
            if (parentNode) {
                TRACE( TRACE_DEBUG, "inner unlock (%x)\n", parentNode);
                pthread_rwlock_unlock(&(parentNode->in_rwlock));
            }

            inner_insert_nonfull(node, depth, key, value);
        }

        // Insertion completed, unlocks the node
        TRACE( TRACE_DEBUG, "inner unlock (%x)\n", node);
        pthread_rwlock_unlock(&(node->in_rwlock));

        return was_split;
    }

    
    // Insert node to a non-full node
    void inner_insert_nonfull(InnerNode* node, unsigned depth, 
                              KEY& key, VALUE& value)
    {
        assert(node->num_keys<INODE_ENTRIES);
        assert(depth!=0);
        
        // Simple linear search
        unsigned index = inner_position_for(key, node->keys, node->num_keys);
        InsertionResult result;
        bool was_split;
        
        if ( depth-1 == 0 ) {
            // The children are leaf nodes, lock the leaf child 
            // and insert entry
            LeafNode* leafChild = reinterpret_cast<LeafNode*>
                (node->children[index]);

            TRACE( TRACE_DEBUG, "leaf write (%x)\n", leafChild);
            pthread_rwlock_wrlock(&(leafChild->leaf_rwlock));
            
            was_split = leaf_insert( leafChild, node,
                                     key, value, &result);
        } 
        else {
            // The children are inner nodes, lock the inner child
            // and insert entry
            InnerNode* innerChild = reinterpret_cast<InnerNode*>
                (node->children[index]);
            
            TRACE( TRACE_DEBUG, "inner write (%x)\n", innerChild);
            pthread_rwlock_wrlock(&(innerChild->in_rwlock));

            was_split = inner_insert(innerChild, node, depth-1, 
                                     key, value, &result);
        }
        
        if (was_split) {
            // There was a split to the child node.
            // The node is still locked. It should be updated and
            // then released.

            if ( index == node->num_keys ) {
                // Insertion at the rightmost key
                node->keys[index]= result.key;
                node->children[index]= result.left;
                node->children[index+1]= result.right;
                node->num_keys++;
            } 
            else {
                // Insertion not at the rightmost key
                node->children[node->num_keys+1] = 
                    node->children[node->num_keys];
                
                for (unsigned i=node->num_keys; i!=index; --i) {
                    node->children[i] = node->children[i-1];
                    node->keys[i] = node->keys[i-1];
                }
            
                node->children[index] = result.left;
                node->children[index+1] = result.right;
                node->keys[index] = result.key;
                node->num_keys++;
            }

            TRACE( TRACE_DEBUG, "inner unlock (%x)\n", node);
            pthread_rwlock_unlock(&(node->in_rwlock));
        } 
        // else the current node is not affected 
        // and its lock is already released
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
    pthread_mutex_t depth_mutex;
    unsigned depth;

    // Pointer to the root node. It may be a leaf or an inner node, but
    // it is never null.
    void*    root;

}; // EOF: BPlusTree


#endif // __BPLUSTREE_H

