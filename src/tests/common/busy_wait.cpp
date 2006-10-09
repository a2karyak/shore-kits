
#include "tests/common/busy_wait.h"

#define ARRAY_SIZE 20


void busy_wait(void) {

  int myarray[ARRAY_SIZE];
  int i;
  while (1) {
    for (i = 0; i < ARRAY_SIZE; i++) {
      myarray[i] = 2 * myarray[i];
    }
  }
}
