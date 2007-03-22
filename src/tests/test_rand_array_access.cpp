/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"
#include "tests/common/cpu_bind.h"



int access_random_entries(int* array, int size) {

    assert(array != NULL);
    assert(size > 0);
    thread_t* this_thread = thread_get_self();

    int total = 0;

    int num_tries=0;
    while(total != 1001) { // try to outsmart compiler...
        int index = this_thread->rand() % size;
        total += array[index];
        num_tries++;
    }
    
    TRACE(TRACE_ALWAYS, "Error! Exited loop!\n");
    assert(0);
    return num_tries;
}


void fill_array_with_random_numbers(int* array, int size) {

    assert(array != NULL);
    assert(size > 0);
    thread_t* this_thread = thread_get_self();

    int i;
    for (i = 0; i < size; i++) {
        array[i] = this_thread->rand();
    }
}


int main(int argc, char* argv[]) {

    util_init();

    // bind to CPU 0
    bind_to_cpu(0);


    // parse array size (bytes)
    if ( argc < 2 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <array size (bytes)>\n", argv[0]);
	exit(-1);
    }
    int array_bytes = atoi(argv[1]);
    if ( array_bytes <= 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid array size %s\n", argv[2]);
	exit(-1);
    }


    int num_ints = array_bytes / sizeof(int);
    array_guard_t<int> array = new int[num_ints];
    fill_array_with_random_numbers(array, num_ints);

    TRACE(TRACE_ALWAYS, "Returned %d\n",
          access_random_entries(array, num_ints));
    
    return 0;
}
