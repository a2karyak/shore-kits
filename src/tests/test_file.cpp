/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file test_file.h
 *
 *  @brief Test of include/file.h
 */

#include "util/file.h"

using namespace std;


void write_to(const char* fname) {
    // open file for appending (acquire resource)
    file logfile(fname, "a+");
    
    logfile.write("hello logfile!");
    logfile.flush();
}


void read_from(const char* fname) {
    // open file for reading (acquire resource)
    file logfile(fname);

    size_t len = 80;
    char* buf = new char[len];

    for (int i=0; logfile.read(buf,len); i++) {
        cout << i << " " << buf << endl;
    }

    delete [] buf;
}


int main(int argc, char* argv[]) {

    write_to("ippo.txt");
    cout << "wrote..." << endl;

    read_from("ippo.txt");
    cout << "read..." << endl;

  return (0);
}
