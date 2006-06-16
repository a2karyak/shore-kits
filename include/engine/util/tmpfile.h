#ifndef __TMPFILE_H
#define __TMPFILE_H

#include <cstdio>
#include <string>

using std::string;

FILE *create_tmp_file(string &name, const string &prefix);

#endif
