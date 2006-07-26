/* -*- mode:C++; c-basic-offset:4 -*- */

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <climits>

#include "trace.h"



#define BUFFER_SIZE 1024

static int  _start    = 0;
static int  _interval = 0;
static int* _counts = NULL;

static bool next_fgets();


    

int main(int argc, char* argv[]) {
    
    // parse command line args
    if ( argc < 2 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <interval> [max]\n", argv[0]);
	exit(-1);
    }

    _interval = atoi(argv[1]);
    if ( _interval == 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid interval %s\n", argv[1]);
	exit(-1);
    }
    
    int max_value = 0;
    if ( argc >= 3 ) {
        max_value = atoi(argv[2]);
        if ( max_value == 0 ) {
            TRACE(TRACE_ALWAYS, "Invalid max value %s\n", argv[2]);
            exit(-1);
        }
    }
    
    // allocate count buckets...
    int num_buckets = ((max_value) ? (max_value) : INT_MAX) / _interval;
    _counts = (int*)calloc( num_buckets, sizeof(int) );
    

    bool running = true;
    while (running)
        running = next_fgets();

    
    for (int i = 0; i < num_buckets; i++)
        printf("%d\n", _counts[i]);

    
    return 0;
}



static bool process_data(const char* data) {

    int num   = atoi(data);
    int diff  = num - _start;
    int index = diff / _interval;

    //    TRACE(TRACE_ALWAYS, "Number %d added to bucket %d\n", num, index);
    
    _counts[index]++;
    return true;
}    



static bool next_fgets() {

    char data[BUFFER_SIZE];
    if ( fgets(data, sizeof(data), stdin) == NULL )
        // EOF
        return false;


    // chomp off trailing '\n', if it exists
    int len = strlen(data);
    if ( data[len-1] == '\n' )
        data[len-1] = '\0';
    

    if ( !process_data(data) )
        // quit/exit data
        return false;

    // continue...
    return true;
}
