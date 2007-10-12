/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file test_bptree.cpp
 *
 *  @brief Test of the In-memory B+ tree implementaion 
 *  (loading, insterting, updating, multi-threaded)
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "util/bptree.h"

// For random number generator (rng)
#include <boost/random.hpp>

// DEBUG
#include <iostream>
using std::cout;
using std::endl;


int main(void)
{
  // How many keys to produce
  const int MAX_KEYS = 100000;

  std::cout << "Hello world!\n";
  

  const unsigned cTwoLines = 128;
  const unsigned cINodeEntries = 7;
  const unsigned cLeafEntries = 14;
  const unsigned cArch = 64;

  BOOST_STATIC_ASSERT(cTwoLines - 16 * cINodeEntries - 8);

  const unsigned cINodePad = cTwoLines - 16 * cINodeEntries - 8;
  const unsigned cLeafPad = cTwoLines - 8 * cLeafEntries - 8;

  assert (cINodePad > 0);
  assert (cLeafPad > 0);

  BPlusTree<int, int, cINodeEntries, cLeafEntries, cINodePad, cLeafPad, cArch> aTree;

  boost::rand48 rng(boost::int32_t(2137));

  cout << "IN= " << aTree.sizeof_inner_node() << " LE= " << aTree.sizeof_leaf_node() << "\n";

  // Load MAX_KEYS keys
  int avalue = 0;
  for (int i = 0; i < MAX_KEYS; i++) {
    avalue = rng();
    aTree.insert(i, avalue);

    cout << i << " " << avalue << "\n";
  }

  int anotherValue = 0;
  for (int i = 0; i < MAX_KEYS; i++) {
    if (aTree.find(i, &anotherValue))
      cout << "Found (" << i << ") V = " << anotherValue << "\n";
  }

  return (0);
}
