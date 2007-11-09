/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   bptree.h
 *
 *  @brief:  Declaration of an in-memory B+ tree with latches class. 
 *
 *  @note:   This B+ tree is append only. Deletions are not implemented.
 *
 *  @author: Ippokratis Pandis (ipandis)
 *
 *  @history
 *  11/9/07:  Option to lock all nodes as it goes down the tree and returns 
 *            pointers to the locks in a vector 
 *  10/20/07: Adding a root rwlock
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

    ////////////////////////////////////////////////////////////////
    // B+ tree Nodes data structures 
    ////////////////////////////////////////////////////////////////


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


    // Name
    c_str _name;

public:

    ////////////////////////////////////////////////////////////////
    // B+ tree exported functions 
    ////////////////////////////////////////////////////////////////


    // INODE_ENTRIES must be greater than two to make the split of
    // two inner nodes sensible.
    BOOST_STATIC_ASSERT(INODE_ENTRIES>2);
    
    // Leaf nodes must be able to hold at least one element
    BOOST_STATIC_ASSERT(LEAF_ENTRIES>0);


    /** @fn:   Constructor
     *  @desc: Builds a new empty tree.
     */

    BPlusTree(c_str aTreeName = c_str("atree"))
        : _name(aTreeName), _depth(0)
    {
        // Initialization of mutexes
        pthread_mutex_init(&_innerPool_mutex, NULL);
        pthread_mutex_init(&_leafPool_mutex, NULL);
        pthread_rwlock_init(&_root_rwlock, NULL);

        // Initialization of the root, should be done after mutex init
        _root = new_leaf_node();
    }



    /** @fn:   Destructor 
     *  @desc: Destroys the tree.
     */

    ~BPlusTree() 
    {
        // Memory deallocation is done automatically
        // when innerPool and leafPool are destroyed.

        // Destroes the mutexes
        pthread_mutex_destroy(&_innerPool_mutex);
        pthread_mutex_destroy(&_leafPool_mutex);
        pthread_rwlock_destroy(&_root_rwlock);
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

        // Before doing anything grabs the root mutex for writing
        // If no-split will happen the lock will be released by the 
        // consequent leaf_insert() or inner_insert() functions
        pthread_rwlock_wrlock(&_root_rwlock);
        // TRACE( TRACE_DEBUG, "root write-lock\n");

        if ( _depth == 0 ) {
            // The root is a leaf node 
            rootLeafProxy = reinterpret_cast<LeafNode*>(_root);
            was_split = leaf_insert(rootLeafProxy, NULL, key, value, &result);
        } 
        else {
            // The root is an inner node
            rootProxy = reinterpret_cast<InnerNode*>(_root);
            was_split = inner_insert(rootProxy, NULL,
                                     _depth, key, value, &result);
        }

        if (was_split) {
            // The old root was splitted in two parts.
            // We have to create a new root pointing to the new sub-nodes.
            // Once we are done we should unlock the old root.
            // Also since there was a split that means that the root mutex is
            // still locked.

            // Create the new root node
            _root = new_inner_node();
            InnerNode* newRootProxy = reinterpret_cast<InnerNode*>(_root);
            newRootProxy->num_keys = 1;
            newRootProxy->keys[0] = result.key;
            newRootProxy->children[0] = result.left;
            newRootProxy->children[1] = result.right;

            // The tree just got one level taller
            _depth++;
            TRACE( TRACE_ALWAYS, "(%s) Depth = %d ***\n", _name.data(), _depth);

            // Unlocks the root lock
            // TRACE( TRACE_DEBUG, "root unlock\n");
            pthread_rwlock_unlock(&_root_rwlock);


            // Prints the whole tree for debug
            //print();
        }
    }

    

    /** @fn:   find
     *  @desc: Looks for the given key. If it is not found, it returns false,
     *  if it is found, it returns true and copies the associated value
     *  unless the pointer is null.
     */

    bool find(const KEY& key, VALUE* value = 0)  
    {
        // Before doing anything grabs the root mutex for reading
        // Once the root lock is acquired the root pointer and depth
        // are reliable value.
        // The root lock will be released as soon we acquire the read
        // lock for any child node
        pthread_rwlock_rdlock(&_root_rwlock);
        // TRACE( TRACE_DEBUG, "root read-lock\n");

        void* aNode = _root; 
        InnerNode* inner = NULL;
        InnerNode* parentInner = NULL;
        uint d = _depth;
        uint index = 0;

        // Drills down the InnerNodes
        while ( d-- > 0 ) {
            assert(aNode);
            
            inner = reinterpret_cast<InnerNode*>(aNode);

            // Read locks the node
            pthread_rwlock_rdlock(&(inner->in_rwlock));
            // TRACE( TRACE_DEBUG, "inner read (%p)\n", aNode);

            // Now that it locked the child it should unlock the parent
            if (parentInner) {
                // TRACE( TRACE_DEBUG, "inner unlock (%p)\n", parentInner);
                pthread_rwlock_unlock(&(parentInner->in_rwlock));
            }
            else {
                // If no parent it means that we are at the root inner node
                // Since we acquired the read lock to that node we can 
                // release the root lock
                // TRACE( TRACE_DEBUG, "root unlock\n");
                pthread_rwlock_unlock(&_root_rwlock);
            }

            // Find the correct child node and save pointer to the current node 
            // in order to release it in the next iteration
            index = inner_position_for(key, inner->keys, inner->num_keys);
            aNode = inner->children[index];
            parentInner = inner;
        }
        
        // Reached the correct leaf node
        assert(aNode);

        // Searches within the leaf node
        LeafNode* leaf = reinterpret_cast<LeafNode*>(aNode);

        // Read lock the leaf node
        pthread_rwlock_rdlock(&(leaf->leaf_rwlock));
        // TRACE( TRACE_DEBUG, "leaf read (%p)\n", aNode);

        // Now that it locked the leaf it should unlock the parent inner node
        if (parentInner) {
            // TRACE( TRACE_DEBUG, "inner unlock (%p)\n", parentInner);
            pthread_rwlock_unlock(&(parentInner->in_rwlock));
        }
        else {
            // If no parent it means that we are at the root inner node
            // Since we acquired the read lock to that node we can 
            // release the root lock
            // TRACE( TRACE_DEBUG, "root unlock\n");
            pthread_rwlock_unlock(&_root_rwlock);
        }

        // Look for the position of the queried entry in the Leaf
        index = leaf_position_for(key, leaf->keys, leaf->num_keys);
        
        // Checks if valid key found and unlocks leaf
        if ( index < leaf->num_keys && leaf->keys[index] == key ) {
            // Entry found, copies the value and returns true
            if ( value != 0 ) {
                *value = leaf->values[index];
            }

            // TRACE( TRACE_DEBUG, "leaf unlock (%p)\n", leaf);
            pthread_rwlock_unlock(&(leaf->leaf_rwlock));

            return true;
        } 
        else {
            // No entry with that key found, returns false
            // TRACE( TRACE_DEBUG, "leaf unlock (%p)\n", leaf);
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
        return (_depth);
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

    void print() 
    {
        int entryCounter = 0;

        // Locks the root       
        pthread_rwlock_rdlock(&_root_rwlock);
        // TRACE( TRACE_DEBUG, "root read\n");

        if (_depth == 0) {
            // the tree consists only by a root/leaf node
            printLeaf(reinterpret_cast<LeafNode*>(_root), &entryCounter, 0);
        }
        else {
            // the root is an inner node
            printInner(reinterpret_cast<InnerNode*>(_root), &entryCounter, _depth-1);
        }            

        // Unlocks the root
        // TRACE( TRACE_DEBUG, "root unlock\n");
        TRACE( TRACE_ALWAYS, "Tree: (%s). Levels: (%d). Entries: (%d)\n",
               _name.data(), _depth, entryCounter);
        pthread_rwlock_unlock(&_root_rwlock);


    }


    /** @fn:    dump
     */

    void dump() 
    {
        print();
    }


    /** @fn:    set_name
     *  @brief: Sets the name of the Tree
     */ 

    void set_name(c_str aNewName) { _name = aNewName; }

    
    /** @fn:    set_name
     *  @brief: Sets the name of the Tree
     */ 

    const void get_name() { return(_name); }



    /*
    // Overrides operator<< (for ostreams)
    template <typename AKEY, typename AVALUE, 
              uint AINODE_ENTRIES, uint ALEAF_ENTRIES,
              uint AINODE_PADDING, uint ALEAF_PADDING,
              uint ANODE_ALIGNMENT>
    friend std::ostream& operator<<(std::ostream& out, 
                                    BPlusTree<AKEY, AVALUE, AINODE_ENTRIES, ALEAF_ENTRIES,
                                    AINODE_PADDING, ALEAF_PADDING, ANODE_ALIGNMENT>& bpt)
    {
        // Data types sizes
        out << "Data types sizes\n";
        out << "uint             = " << sizeof(uint) << "\n";
        out << "pthread_rwlock_t = " << sizeof(pthread_rwlock_t) << "\n";
        out << "void*            = " << sizeof(void*) << "\n";
        out << "uchar            = " << sizeof(unsigned char) << "\n";
        out << "pthread_mutex_t  = " << sizeof(pthread_mutex_t) << "\n";

        out << "InnerPool        = " << sizeof(bpt._innerPool) << "\n";
        out << "LeafPool         = " << sizeof(bpt._leafPool) << "\n";

        out << "KEY              = " << sizeof(AKEY) << "\n";
        out << "VALUE            = " << sizeof(AVALUE) << "\n";


        // Tree description
        out << "bpt: " << sizeof(bpt) 
            << "\nINNER: ENTRIES = " << AINODE_ENTRIES
            << "\tPAD = " << AINODE_PADDING
            << "\tSZ = " << bpt.sizeof_inner_node() 
            << "\nLEAF : ENTRIES = " << ALEAF_ENTRIES
            << "\tPAD = " << ALEAF_PADDING           
            << "\tSZ = " << bpt.sizeof_leaf_node()  
            << "\n";
        
        return (out);
    }
    */

    ////////////////////////////////////////////////////////////////


private:


    ////////////////////////////////////////////////////////////////
    // Debug Helper functions 
    ////////////////////////////////////////////////////////////////


    /** @fn:   printInner
     *  @desc: Dumps the data of an InnerNode
     */

    void printInner(InnerNode* aInnerNode, int* counter, uint localDepth)
    {
        assert (aInnerNode);

        // Read-locks the node
        pthread_rwlock_rdlock(&(aInnerNode->in_rwlock));
        // TRACE( TRACE_DEBUG, "inner read (%p)\n", aInnerNode);

        uint iNodeKeys = aInnerNode->num_keys;
        uint i = 0;

        // TRACE( TRACE_DEBUG, "InnerNode: %p. Level: %d. Entries: %d\n",        
        //               aInnerNode, (localDepth+1), iNodeKeys);

        assert (iNodeKeys <= INODE_ENTRIES);

        // Prints out the Keys of the InnerNode
        for (i=0; i<iNodeKeys; i++) {
            /*
            // TRACE( TRACE_DEBUG, "PL=(%d) K=(%d)\n", i, 
                   aInnerNode->keys[i]);
            */
        }            

        // Calls the print function for the children        
        if (localDepth == 0) {
            // The children are leaf nodes
            for (i=0; i<iNodeKeys+1; i++) {                
                printLeaf(reinterpret_cast<LeafNode*>(aInnerNode->children[i]), 
                          counter,
                          localDepth);
            }
        }
        else {
            // The children are inner nodes
            for (i=0; i<iNodeKeys+1; i++) {
                printInner(reinterpret_cast<InnerNode*>(aInnerNode->children[i]),
                           counter,
                           localDepth-1);
            }
        }

        // Traversing completed, unlocks the node
        // TRACE( TRACE_DEBUG, "inner read unlock (%p)\n", aInnerNode);
        pthread_rwlock_unlock(&(aInnerNode->in_rwlock));
    }


    /** @fn:   printLeaf
     *  @desc: Dumps the entries in a LeafNode
     */
    
    void printLeaf(LeafNode* aLeafNode, int* counter, uint level) 
    {
        assert (aLeafNode);

        // Read-locks the node
        pthread_rwlock_rdlock(&(aLeafNode->leaf_rwlock));
        // TRACE( TRACE_DEBUG, "leaf read (%p)\n", aLeafNode);

        uint iLeafEntries = aLeafNode->num_keys;
        uint i = 0;

        assert (iLeafEntries <= LEAF_ENTRIES);


        // TRACE( TRACE_DEBUG, "LeafNode: %p. Level: %d. Entries: %d\n",        
        //               aLeafNode, level, iLeafEntries);

        *counter = *counter + iLeafEntries;

        // Prints out the Entries of the LeafNode
        for (i=0; i<iLeafEntries; i++) {
            /*
            // TRACE( TRACE_DEBUG, "PL=(%d) K=(%d) V=(%d)\n", i,
            aLeafNode->keys[i], 
            aLeafNode->values[i]);
            */
        }            

        // Reading completed, unlocks the node
        // TRACE( TRACE_DEBUG, "leaf read unlock (%p)\n", aLeafNode);
        pthread_rwlock_unlock(&(aLeafNode->leaf_rwlock));
    }


    ////////////////////////////////////////////////////////////////
    // Memory management functions 
    ////////////////////////////////////////////////////////////////

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
        pthread_mutex_lock(&_leafPool_mutex);

        //result= new LeafNode();
        result = _leafPool.construct();

        // TRACE( TRACE_DEBUG, "New LeafNode at (%p)\n", result);

        // release leafPool lock
        pthread_mutex_unlock(&_leafPool_mutex);

        return result;
    }
    
    // Frees a leaf node previously allocated with new_leaf_node()
    void delete_leaf_node(LeafNode* node) {

        assert (0); // (ip): Not implemented yet!

        //cout << "Deleting LeafNode at " << node << endl;
        _leafPool.destroy(node);
    }


    /** @fn:   new_inner_node
     *  @desc: Returns a pointer to a fresh inner node, locked for writing
     *
     *  @note: Uses the innerNodePool for this. Made thread_safe.
     */

    InnerNode* new_inner_node() 
    {
        InnerNode* result;

        // regulate the access to the innerPool
        pthread_mutex_lock(&_innerPool_mutex);

        result = _innerPool.construct();

        // TRACE( TRACE_DEBUG, "New InnerNode at (%p)\n", result);
            
        // release innerPool lock
        pthread_mutex_unlock(&_innerPool_mutex);
        return result;
    }

    // Frees an inner node previously allocated with new_inner_node()
    void delete_inner_node(InnerNode* node) {

        assert (0); // (ip): Not implemented yet!

        //cout << "Deleting InnerNode at " << node << endl;
        _innerPool.destroy(node);
    }



    ////////////////////////////////////////////////////////////////
    // B+ tree functionality functions 
    ////////////////////////////////////////////////////////////////

        
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
     */

    bool leaf_insert(LeafNode* node, InnerNode* parentNode,
                     KEY& key, VALUE& value, 
                     InsertionResult* result) 
    {
        assert(node);

        // Write-Locks the leaf node before insertion
        // The lock will be released inside the leaf_insert_nonfull()
        pthread_rwlock_wrlock(&(node->leaf_rwlock));
        // TRACE( TRACE_DEBUG, "leaf write (%p)\n", node);

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
            
            node->num_keys = treshold;
        
            if (i<treshold) {
                // Entry should go to left sibling
                leaf_insert_nonfull(node, key, value, i);
            } 
            else {
                // Entry should go to right sibling
                // The new_sibling is not locked, we (unnecessarily) lock it
                // in order to treat all the leaf nodes the same. 
                // The new_sibling will be unlocked inside the leaf_insert_nonfull()
                // We also unlock the old Leaf node.

                pthread_rwlock_wrlock(&(new_sibling->leaf_rwlock));
                // TRACE( TRACE_DEBUG, "leaf write (%p)\n", new_sibling);

                // TRACE( TRACE_DEBUG, "leaf unlock (%p)\n", node);
                pthread_rwlock_unlock(&(node->leaf_rwlock));

                leaf_insert_nonfull(new_sibling, key, value, i-treshold);
            }
            
            // Notify the parent about the split
            was_split = true;
            result->key = new_sibling->keys[0];
            result->left = node;
            result->right = new_sibling;
        } 
        else {            
            // The Leaf node is not full. No split happens.
            // Can release parent.
            if (parentNode) {
                // Normal Leaf with a parent InnerNode.
                // The lock to the InnerNode can be released.
                // TRACE( TRACE_DEBUG, "inner unlock (%p)\n", parentNode);
                pthread_rwlock_unlock(&(parentNode->in_rwlock));
            }
            else {
                // The current Leaf is the root. 
                // Since it will not change we can release the root lock
                // TRACE( TRACE_DEBUG, "root unlock\n");
                pthread_rwlock_unlock(&_root_rwlock);
            }

            leaf_insert_nonfull(node, key, value, i);
        }
        
        return was_split;        
    }
    

    /** @fn:   leaf_insert_nonfull 
     *  @desc: Inserts an entry to a non-full leaf node. 
     *
     *  @note: The Leaf node is already locked by the thread that does the
     *  insertion. This operation should never fail.
     *  @note: The lock of the Leaf node is released here.
     */

    void leaf_insert_nonfull(LeafNode* node, KEY& key, 
                             VALUE& value, uint index)
    {
        assert(node->num_keys<LEAF_ENTRIES);
        assert(index<=node->num_keys);
        
        if ( (index < node->num_keys) && 
             (node->keys[index] == key)) {
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

        /*
        // print debug info
        // TRACE( TRACE_DEBUG, "\nEntry: K=(%d) V=(%d). At Leaf=(%p) IDX=(%d) #KEYS=(%d)\n",
        key,value, node, index, node->num_keys);
        */

        // Insertion completed, unlocks the node
        // TRACE( TRACE_DEBUG, "leaf unlock (%p)\n", node);
        pthread_rwlock_unlock(&(node->leaf_rwlock));
    }


    /** @fn:   inner_insert 
     *  @desc: Inserts an entry to an inner node. If there not enough room
     *  it causes an inner node split. 
     *  @note: When this function returns the node will be unlocked. The lock
     *  is released by the inner_insert_nonfull()
     */

    bool inner_insert(InnerNode* node, InnerNode* parentNode, uint localDepth, 
                      KEY& key, VALUE& value, 
                      InsertionResult* result)
    {
        assert(node);

        // Write-Locks the inner node before insert
        pthread_rwlock_wrlock(&(node->in_rwlock));
        // TRACE( TRACE_DEBUG, "inner write (%p)\n", node);
        
        // Since we are inserting to an inner node the localDepth > 0
        assert(localDepth>0);
        
        // Early split if node is full.
        // This is not the canonical algorithm for B+ trees,
        // but it is simpler and does not break the definition.
        bool was_split = false;
        
        if ( node->num_keys == INODE_ENTRIES ) {
            // Split the inner node and transfer threshold number of entries to the new
            // sibling node
            // @note: The sibling node does not need locking because nobody knows its
            // existence, but we will lock it anyways to have the same treatment
            // for  all the nodes.
            uint treshold = (INODE_ENTRIES+1)/2;
            InnerNode* new_sibling = new_inner_node();
            
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
                inner_insert_nonfull(node, localDepth, key, value);
            } 
            else {
                // Inserting to the newly created (sibling) node
                // The new_sibling is not locked, we (unnecessarily) lock it
                // in order to treal all the inner nodes the same.
                // The old node will not be modified anymore, so we can release it

                pthread_rwlock_wrlock(&(new_sibling->in_rwlock));
                // TRACE( TRACE_DEBUG, "inner write (%p)\n", new_sibling);

                // TRACE( TRACE_DEBUG, "inner unlock (%p)\n", node);
                pthread_rwlock_unlock(&(node->in_rwlock));                
                
                inner_insert_nonfull(new_sibling, localDepth, key, value);
            }
        } 
        else {
            // The InnerNode is not full. No split happens.
            // Can release parent.
            if (parentNode) {
                // Normal InnerNode with a parent InnerNode
                // TRACE( TRACE_DEBUG, "inner unlock (%p)\n", parentNode);
                pthread_rwlock_unlock(&(parentNode->in_rwlock));
            }
            else {
                // The current InnerNode is the root.
                // Since it will not change we can release the root lock.
                // TRACE( TRACE_DEBUG, "root unlock\n");
                pthread_rwlock_unlock(&_root_rwlock);
            }                

            inner_insert_nonfull(node, localDepth, key, value);
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

    void inner_insert_nonfull(InnerNode* node, uint localDepth, 
                              KEY& key, VALUE& value)
    {
        assert(node->num_keys<INODE_ENTRIES);
        assert(localDepth>0);
        
        // Simple linear search
        uint index = inner_position_for(key, node->keys, node->num_keys);
        InsertionResult result;
        bool was_split;
        
        if ( localDepth-1 == 0 ) {
            // The children are leaf nodes, lock the leaf child 
            // and insert entry            
            was_split = leaf_insert( reinterpret_cast<LeafNode*>(node->children[index]),
                                     node, key, value, &result);
        } 
        else {
            // The children are inner nodes
            was_split = inner_insert( reinterpret_cast<InnerNode*>(node->children[index]),
                                      node, localDepth-1, key, value, &result);
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
            // TRACE( TRACE_DEBUG, "inner unlock (%p)\n", node);
            pthread_rwlock_unlock(&(node->in_rwlock));
        } 

        // @note: If there was not a split the node has already been released
        // by its child in the body of the leaf_insert() or inner_insert() 
        // functions
    }



    ////////////////////////////////////////////////////////////////
    // B+ tree member variables 
    ////////////////////////////////////////////////////////////////


    typedef AlignedMemoryAllocator<NODE_ALIGNMENT> AlignedAllocator;
    
    // Node memory allocators. IMPORTANT @NOTE: they must be declared
    // before the root to make sure that they are properly initialised
    // before being used to allocate any node
    pthread_mutex_t _innerPool_mutex;
    boost::object_pool<InnerNode, AlignedAllocator> _innerPool;
    pthread_mutex_t _leafPool_mutex;
    boost::object_pool<LeafNode, AlignedAllocator>  _leafPool;
    
    // Depth of the tree. A tree of depth 0 only has a leaf node.
    uint _depth;

    // Pointer to the root node. It may be a leaf or an inner node, but
    // it is never null
    pthread_rwlock_t _root_rwlock;
    void*            _root;



}; // EOF: BPlusTree


#endif // __BPLUSTREE_H

