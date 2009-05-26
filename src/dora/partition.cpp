/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   partition.cpp
 *
 *  @brief:  Helper functions for DORA partitions
 *           The main functionality is on the header file.
 *
 *  @author: Ippokratis Pandis, Nov 2008
 */


#include "dora/partition.h"
#include "dora/action.h"


ENTER_NAMESPACE(dora);


struct pretty_printer {
    ostringstream _out;
    string _tmp;
    operator ostream&() { return _out; }
    operator char const*() { _tmp = _out.str(); _out.str(""); return _tmp.c_str(); }
};


static void _print_partition(std::ostream &out, const partition_t<int> &part) 
{
  out << "Partition: " << part.table()->name() << "-" << part.part_id() << endl;
  out << "Input: " << !part.has_input() << endl; 
  out << "Commit: " << !part.has_committed() << endl; 
}

char const* db_pretty_print(const partition_t<int>* part, int i=0, char const* s=0) 
{
    static pretty_printer pp;
    _print_partition(pp, *part);
    return pp;
}


EXIT_NAMESPACE(dora);

