/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   bptree.h
 *
 *  @brief:  Declaration of an in-memory B+ tree with latches class
 *
 *  @author: Ippokratis Pandis (ipandis)
 *
 *  @history
 *  10/17/07: Implementing a "crabbing" locking schema
 *  10/16/07: Adding Reader-Writer Locks to the nodes and shared variables
 */

#ifndef __BPLUSTREE_H
#define __BPLUSTREE_H

#ifdef __SUNPRO_CC
#include <stdlib.h>
#else
// This is required for glibc to define std::posix_memalign
#define _XOPEN_SOURCE 600
#include <cstdlib>
#endif

// Boost includes
#include <boost/static_assert.hpp>
#include <boost/pool/object_pool.hpp>

#include "util/sync.h"
#include "util/trace.h"

typedef unsigned int uint;

template <typename KEY, typename VALUE, 
          uint INODE_ENTRIES, uint LEAF_ENTRIES,
          uint INODE_PADDING = 0, uint LEAF_PADDING = 0,
          uint NODE_ALIGNMENT = 64>
class BPlusTree
{
private:

    /** @struct: LeafNode
     *  @desc:   A structure for the leaf nodes of the BPTree
     *  @note:   Leaf nodes store pairs of keys and values
     */

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

        uint             num_keys;
        KEY              keys[LEAF_ENTRIES];
        VALUE            values[LEAF_ENTRIES];
        unsigned char    _pad[LEAF_PADDING];
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

        uint             num_keys;
        KEY              keys[INODE_ENTRIES];
        void*            children[INODE_ENTRIES+1];
        unsigned char    _pad[INODE_PADDING];
    };

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

        // Prints the whole tree
        print();
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
        register uint oldDepth = depth;

        if ( oldDepth == 0 ) {
            // The root is a leaf node 
            rootLeafProxy = reinterpret_cast<LeafNode*>(root);

            pthread_rwlock_wrlock(&(rootLeafProxy->leaf_rwlock));
            TRACE( TRACE_DEBUG, "root leaf write-lock (%p)\n", rootLeafProxy);

            was_split = leaf_insert(rootLeafProxy, NULL, key, value, &result);
        } 
        else {
            // The root is an inner node
            rootProxy = reinterpret_cast<InnerNode*>(root);

            pthread_rwlock_wrlock(&(rootProxy->in_rwlock));
            TRACE( TRACE_DEBUG, "root inner write-lock (%p)\n", rootProxy);

            was_split = inner_insert(rootProxy, NULL,
                                     oldDepth, key, value, &result);
        }

        if (was_split) {
            // The old root was splitted in two parts.
            // We have to create a new root pointing to them
            // Once we are done we should unlock the old root.
            //pthread_mutex_lock(&depth_mutex);

            // Create the new root node
            root = new_inner_node(true);
            InnerNode* newRootProxy = reinterpret_cast<InnerNode*>(root);
            newRootProxy->num_keys = 1;
            newRootProxy->keys[0] = result.key;
            newRootProxy->children[0] = result.left;
            newRootProxy->children[1] = result.right;
            
            // Unlock the old root
            if (oldDepth == 0) {
                // the splitted root was a leaf
                TRACE( TRACE_DEBUG, "old-root leaf unlock (%p)\n", rootLeafProxy);
                pthread_rwlock_unlock(&(rootLeafProxy->leaf_rwlock));
            }
            else {
                // the splitted root was a inner node
                TRACE( TRACE_DEBUG, "old-root inner unlock (%p)\n", rootProxy);
                pthread_rwlock_unlock(&(rootProxy->in_rwlock));
            }

            depth++;
            TRACE( TRACE_ALWAYS, "Depth = %d\n", depth);
            //pthread_mutex_unlock(&depth_mutex);

            TRACE( TRACE_DEBUG, "new root unlock (%p)\n", newRootProxy);
            pthread_rwlock_unlock(&(newRootProxy->in_rwlock));

            // Prints the whole tree
            print();
        }
    }
    

    // Looks for the given key. If it is not found, it returns false,
    // if it is found, it returns true and copies the associated value
    // unless the pointer is null.
    bool find(const KEY& key, VALUE* value = 0) const 
    {
        InnerNode* inner;
        register void* node = root;
        uint initDepth = depth;
        register uint d = initDepth;
        register uint index;
    
        // Drills down the InnerNodes
        while ( d-- != 0 ) {
            assert(node);
            
            inner = reinterpret_cast<InnerNode*>(node);

            // Read locks the node
            pthread_rwlock_rdlock(&(inner->in_rwlock));
            TRACE( TRACE_DEBUG, "inner read (%p)\n", node);

            index = inner_position_for(key, inner->keys, inner->num_keys);
            node = inner->children[index];

            // Unlocks the node
            TRACE( TRACE_DEBUG, "inner unlock (%p)\n", node);
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

        // Read lock the leaf node
        pthread_rwlock_rdlock(&(leaf->leaf_rwlock));
        TRACE( TRACE_DEBUG, "leaf read (%p)\n", node);

        index= leaf_position_for(key, leaf->keys, leaf->num_keys);
        
        if ( leaf->keys[index] == key ) {
            // Entry found, copies the value and returns true
            if ( value != 0 ) {
                *value= leaf->values[index];
            }

            TRACE( TRACE_DEBUG, "leaf unlock (%p)\n", node);
            pthread_rwlock_unlock(&(leaf->leaf_rwlock));

            return true;
        } 
        else {
            // No entry with that key found, returns false
            TRACE( TRACE_DEBUG, "leaf unlock (%p)\n", node);
            pthread_rwlock_unlock(&(leaf->leaf_rwlock));

            return false;
        }
    }
    

    ////////////////////////////////////////////////////////////////
    // Helper functions 
    ////////////////////////////////////////////////////////////////

    /** @fn:   get_depth
     *  @desc: Returns the depth of the tree
     */

    uint get_depth() const 
    {
        return (depth);
    }


    /** @fn:   sizeof_inner_node
     *  @desc: Returns the size of an inner node
     *  @note: It is useful when optimizing performance with cache alignment.
     */

    uint sizeof_inner_node() const 
    {
        return sizeof(InnerNode);
    }


    /** @fn:   sizeof_inner_node
     *  @desc: Returns the size of an leaf node
     *  @note: It is useful when optimizing performance with cache alignment.
     */

    uint sizeof_leaf_node() const 
    {
        return sizeof(LeafNode);
    }        


    /** @fn:   print
     *  @desc: Dumps all the data (nodes&entries) in the BPlusTree
     */

    void print() {
        if (depth == 0) {
            // the tree consists only by a root/leaf node
            printLeaf(reinterpret_cast<LeafNode*>(root), 0);
        }
        else {
            // the root is an inner node
            printInner(reinterpret_cast<InnerNode*>(root), depth-1);
        }            
    }


    /** @fn:   printInner
     *  @desc: Dumps the data of an InnerNode
     */

    void printInner(InnerNode* aInnerNode, uint localDepth)
    {
        assert (aInnerNode);

        // Read-locks the node
        pthread_rwlock_rdlock(&(aInnerNode->in_rwlock));
        TRACE( TRACE_DEBUG, "inner read (%p)\n", aInnerNode);

        uint iNodeKeys = aInnerNode->num_keys;
        uint i = 0;

        TRACE( TRACE_DEBUG, "InnerNode: %p. Level: %d. Entries: %d\n",        
               aInnerNode, (localDepth+1), iNodeKeys);

        assert (iNodeKeys <= INODE_ENTRIES);

        // Prints out the Keys of the InnerNode
        for (i=0; i<iNodeKeys; i++) {
            TRACE( TRACE_DEBUG, "PL=(%d) K=(%d)\n", i, aInnerNode->keys[i]);
        }            

        // Calls the print function for the children        
        if (localDepth == 0) {
            // The children are leaf nodes
            for (i=0; i<iNodeKeys+1; i++) {
                printLeaf(reinterpret_cast<LeafNode*>(aInnerNode->children[i]), 
                          localDepth);
            }
        }
        else {
            // The children are inner nodes
            for (i=0; i<iNodeKeys+1; i++) {
                printInner(reinterpret_cast<InnerNode*>(aInnerNode->children[i]),
                           localDepth-1);
            }
        }

        // Traversing completed, unlocks the node
        TRACE( TRACE_DEBUG, "inner read unlock (%p)\n", aInnerNode);
        pthread_rwlock_unlock(&(aInnerNode->in_rwlock));
    }


    /** @fn:   printLeaf
     *  @desc: Dumps the entries in a LeafNode
     */
    
    void printLeaf(LeafNode* aLeafNode, uint level) 
    {
        assert (aLeafNode);

        // Read-locks the node
        pthread_rwlock_rdlock(&(aLeafNode->leaf_rwlock));
        TRACE( TRACE_DEBUG, "leaf read (%p)\n", aLeafNode);

        uint iLeafEntries = aLeafNode->num_keys;
        uint i = 0;

        assert (iLeafEntries <= LEAF_ENTRIES);


        TRACE( TRACE_DEBUG, "LeafNode: %p. Level: %d. Entries: %d\n",        
               aLeafNode, level, iLeafEntries);

        // Prints out the Entries of the LeafNode
        for (i=0; i<iLeafEntries; i++) {
            TRACE( TRACE_DEBUG, "PL=(%d) K=(%d) V=(%d)\n", 
                   i, aLeafNode->keys[i],  aLeafNode->values[i]);
        }            

        // Reading completed, unlocks the node
        TRACE( TRACE_DEBUG, "leaf read unlock (%p)\n", aLeafNode);
        pthread_rwlock_unlock(&(aLeafNode->leaf_rwlock));
    }


    // Overrides operator<< (for ostreams)
    template <typename AKEY, typename AVALUE, 
              uint AINODE_ENTRIES, uint ALEAF_ENTRIES,
              uint AINODE_PADDING, uint ALEAF_PADDING,
              uint ANODE_ALIGNMENT>
    friend std::ostream& operator<<(std::ostream& out, 
                                    BPlusTree<AKEY, AVALUE, AINODE_ENTRIES, ALEAF_ENTRIES,
                                    AINODE_PADDING, ALEAF_PADDING, ANODE_ALIGNMENT>& bpt)
    {
        out << "bpt: " << sizeof(bpt) 
            << " IN_SZ= " << bpt.sizeof_inner_node()
            << " LE_SZ= " << bpt.sizeof_leaf_node() << "\n";
        
        return (out);
    }

    ////////////////////////////////////////////////////////////////


private:
    // Custom allocator that returns aligned blocks of memory
    template <uint ALIGNMENT>
    struct AlignedMemoryAllocator {
        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;
        
        static char* malloc(const size_type bytes) {
            void* result;

#ifdef __SUNPRO_CC
            // (ip) Using memalign() instead of posix_memalign()
            if ((result = memalign(ALIGNMENT, bytes)) == NULL) {
                result = 0;
            }
#else
            if (posix_memalign(&result, ALIGNMENT, bytes) != 0) {
                result = 0;
            }
#endif            
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

        TRACE( TRACE_DEBUG, "New LeafNode at (%p)\n", result);

        // release leafPool lock
        pthread_mutex_unlock(&leafPool_mutex);

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

        TRACE( TRACE_DEBUG, "New InnerNode at (%p)\n", result);

        if (bLocked) {
            pthread_rwlock_wrlock(&(result->in_rwlock));
            TRACE( TRACE_DEBUG, "new inner write-lock (%p)\n", result);
        }
            
        // release innerPool lock
        pthread_mutex_unlock(&innerPool_mutex);
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
    inline static uint leaf_position_for(const KEY& key, 
                                             const KEY* keys,
                                             uint num_keys) 
    {
        // Simple linear search. Faster for small values of N or M
        uint k = 0;
        
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
        return uint(right);
        */
    }

    // Returns the position where 'key' should be inserted in an inner node
    // that has the given keys.
    inline static uint inner_position_for(const KEY& key, 
                                              const KEY* keys,
                                              uint num_keys) 
    {
      // Simple linear search. Faster for small values of N or M
        uint k = 0;
        while((k < num_keys) && ((keys[k]<key) || (keys[k]==key))) {
            ++k;
        }
        return k;

        // Binary search is faster when N or M is > 100,
        // but the cost of the division renders it useless
        // for smaller values of N or M.
    }


    /** @fn:   leaf_insert 
     *  @desc: Inserts an entry to a leaf node. If there not enough room
     *  it causes a leaf node split. The thread that inserts an
     *  entry to a leaf node acquires a lock for the whole node.
     *  If no split is to be done the lock to the parent node is released.
     *
     *  @note: It write-locks the target node, unless no parent node
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
            pthread_rwlock_wrlock(&(node->leaf_rwlock));
            TRACE( TRACE_DEBUG, "leaf write (%p)\n", node);
        }

        assert(node->num_keys <= LEAF_ENTRIES);
        bool was_split = false;
        
        // Simple linear search
        uint i = leaf_position_for(key, node->keys, node->num_keys);

        // Check if Leaf node is full
        if ( node->num_keys == LEAF_ENTRIES ) {
            // The node was full. We must split it
            uint treshold = (LEAF_ENTRIES+1)/2;

            // Create a new sibling node and transfer theshold number of entries
            // @note: The sibling node does not need locking because nobody knows its
            // existence
            LeafNode* new_sibling = new_leaf_node();
            new_sibling->num_keys = node->num_keys - treshold;
        
            for(uint j=0; j < new_sibling->num_keys; ++j) {
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
                TRACE( TRACE_DEBUG, "inner unlock (%p)\n", parentNode);
                pthread_rwlock_unlock(&(parentNode->in_rwlock));
            }
                
            // The node was not full
            leaf_insert_nonfull(node, key, value, i);
        }

        // Insertion completed, unlocks the node
        TRACE( TRACE_DEBUG, "leaf unlock (%p)\n", node);
        pthread_rwlock_unlock(&(node->leaf_rwlock));
        
        return was_split;        
    }
    

    /** @fn:   leaf_insert_nonfull 
     *  @desc: Inserts an entry to a non-full leaf node. 
     *
     *  @note: The node is already locked by the thread that does the
     *  insertion. This operation should never fail.
     */

    void leaf_insert_nonfull(LeafNode* node, KEY& key, 
                             VALUE& value, uint index)
    {
        assert(node->num_keys<LEAF_ENTRIES);
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
            for (uint i=node->num_keys; i > index; --i) {
                node->keys[i] = node->keys[i-1];
                node->values[i] = node->values[i-1];
            }
        
            // Now we can go ahead and do the insertion
            node->keys[index]= key;
            node->values[index]= value;
            node->num_keys++;
        }
    }


    /** @fn:   inner_insert 
     *  @desc: Inserts an entry to an inner node. If there not enough room
     *  it causes an inner node split. 
     *
     *  @note: It write-locks the target node, unless no parent node
     *  @note: If inserts to the root node, this node must already be write-locked
     */

    bool inner_insert(InnerNode* node, InnerNode* parentNode, uint depth, 
                      KEY& key, VALUE& value, 
                      InsertionResult* result)
    {
        assert(node);

        // Write-Locks the inner node before insert, unless there is no
        // parentNode. If there is no parentNode it means that the 
        // current node is a inner/root node and it is already locked
        // so there is no need for additional locking
        if (parentNode) {
            pthread_rwlock_wrlock(&(node->in_rwlock));
            TRACE( TRACE_DEBUG, "inner write (%p)\n", node);
        }
        
        // Since we are inserting to an inner node the depth > 0
        assert(depth>0);

        // Sanity check. 
        // If there is no parentNode it means that we are inserting to the root.
        // However, the root may have changed in the meantime. Thus, we have to 
        // change the target node.        
        if ( (parentNode == NULL) && (reinterpret_cast<InnerNode*>(root) != node) ) {

            // Unlock the current node and essentially start the insertion from
            // the beginning. 
            InnerNode* newRootProxy = reinterpret_cast<InnerNode*>(root);
            pthread_rwlock_wrlock(&(newRootProxy->in_rwlock));
            TRACE( TRACE_DEBUG, "root inner write (%p)\n", newRootProxy);
           
            // (ip) TODO: which should be done first? to unlock the old
            // root or to lock the new root?
            TRACE( TRACE_DEBUG, "*** root changed while waiting to insert\n");
            TRACE( TRACE_DEBUG, "inner unlock (%p)\n", node);
            pthread_rwlock_unlock(&(node->in_rwlock));
            
            // restart insertion
            return (inner_insert(newRootProxy, NULL, get_depth(), key, value, result));
        }

        
        // Early split if node is full.
        // This is not the canonical algorithm for B+ trees,
        // but it is simpler and does not break the definition.
        bool was_split = false;
        
        if ( node->num_keys == INODE_ENTRIES ) {
            // Split the inner node and transfer threshold number of entries to the new
            // sibling node
            //
            // @note: The sibling node does not need locking because nobody knows its
            // existence, but we will lock it anyways to have the same treatment for all the
            // nodes.
            uint treshold = (INODE_ENTRIES+1)/2;
            InnerNode* new_sibling = new_inner_node(true);
            
            new_sibling->num_keys = node->num_keys - treshold;
            
            for (uint i=0; i < new_sibling->num_keys; ++i) {
                new_sibling->keys[i] = node->keys[treshold+i];
                new_sibling->children[i] = node->children[treshold+i];
            }
            
            new_sibling->children[new_sibling->num_keys] = node->children[node->num_keys];
            node->num_keys = treshold-1;
        
            // Set up the return variable
            was_split = true;
            result->key = node->keys[treshold-1];
            result->left = node;
            result->right = new_sibling;
    
            // Now insert in the appropriate sibling
            if ( key < result->key ) {
                // Inserting to the already created node
                // @note: The new sibling node will not be modified anymore, 
                // so we can release it
                TRACE( TRACE_DEBUG, "inner unlock (%p)\n", new_sibling);
                pthread_rwlock_unlock(&(new_sibling->in_rwlock));                

                inner_insert_nonfull(node, depth, key, value);
            } 
            else {
                // Inserting to the newly created (sibling) node
                // @note: The old node will not be modified anymore, so we can release it
                TRACE( TRACE_DEBUG, "inner unlock (%p)\n", node);
                pthread_rwlock_unlock(&(node->in_rwlock));                
                
                inner_insert_nonfull(new_sibling, depth, key, value);
            }
        } 
        else {
            // No split, can release parent
            if (parentNode) {
                TRACE( TRACE_DEBUG, "inner unlock (%p)\n", parentNode);
                pthread_rwlock_unlock(&(parentNode->in_rwlock));
            }

            inner_insert_nonfull(node, depth, key, value);
        }

        return was_split;
    }

    
    /** @fn:   inner_insert_nonfull 
     *  @desc: Inserts an entry to a non-full inner node. 
     *
     *  @note: The lock to the node is released here
     *  @node: It is released either inside the leaf_insert()/inner_insert() 
     *  function calls of one of its children, or (if there was split to the 
     *  children node) inside this function.
     */

    void inner_insert_nonfull(InnerNode* node, uint depth, 
                              KEY& key, VALUE& value)
    {
        assert(node->num_keys<INODE_ENTRIES);
        assert(depth>0);
        
        // Simple linear search
        uint index = inner_position_for(key, node->keys, node->num_keys);
        InsertionResult result;
        bool was_split;
        
        if ( depth-1 == 0 ) {
            // The children are leaf nodes, lock the leaf child 
            // and insert entry            
            was_split = leaf_insert( reinterpret_cast<LeafNode*>(node->children[index]),
                                     node, key, value, &result);
        } 
        else {
            // The children are inner nodes
            was_split = inner_insert( reinterpret_cast<InnerNode*>(node->children[index]),
                                      node, depth-1, key, value, &result);
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
                
                for (uint i=node->num_keys; i!=index; --i) {
                    node->children[i] = node->children[i-1];
                    node->keys[i] = node->keys[i-1];
                }
            
                node->children[index] = result.left;
                node->children[index+1] = result.right;
                node->keys[index] = result.key;
                node->num_keys++;
            }

            // The insertion completed and the lock is released
            TRACE( TRACE_DEBUG, "inner unlock (%p)\n", node);
            pthread_rwlock_unlock(&(node->in_rwlock));
        } 

        // @note: If there was not a split the node has already been released
        // in the body of the leaf_insert/inner_insert functions
    }

    typedef AlignedMemoryAllocator<NODE_ALIGNMENT> AlignedAllocator;
    
    // Node memory allocators. IMPORTANT @NOTE: they must be declared
    // before the root to make sure that they are properly initialised
    // before being used to allocate any node.
    pthread_mutex_t innerPool_mutex;
    boost::object_pool<InnerNode, AlignedAllocator> innerPool;
    pthread_mutex_t leafPool_mutex;
    boost::object_pool<LeafNode, AlignedAllocator>  leafPool;
    
    // Depth of the tree. A tree of depth 0 only has a leaf node.
    pthread_mutex_t depth_mutex;
    uint depth;

    // Pointer to the root node. It may be a leaf or an inner node, but
    // it is never null.
    void*    root;

}; // EOF: BPlusTree


#endif // __BPLUSTREE_H

