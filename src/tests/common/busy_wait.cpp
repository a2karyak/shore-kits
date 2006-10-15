
#include "tests/common/busy_wait.h"

#define ARRAY_SIZE 10000


void busy_wait(void) {

  int myarray[ARRAY_SIZE];
  int i;
  while (1) {

#define N 40

    for (i = 0; i < ARRAY_SIZE-N; i += N) {


      myarray[i+0] = myarray[i+0]+1;
      myarray[i+1] = myarray[i+1]+1;
      myarray[i+2] = myarray[i+2]+1;
      myarray[i+3] = myarray[i+3]+1;
      myarray[i+4] = myarray[i+4]+1;

      myarray[i+5] = myarray[i+5]+1;
      myarray[i+6] = myarray[i+6]+1;
      myarray[i+7] = myarray[i+7]+1;
      myarray[i+8] = myarray[i+8]+1;
      myarray[i+9] = myarray[i+9]+1;


      myarray[i+10] = myarray[i+10]+1;
      myarray[i+11] = myarray[i+11]+1;
      myarray[i+12] = myarray[i+12]+1;
      myarray[i+13] = myarray[i+13]+1;
      myarray[i+14] = myarray[i+14]+1;

      myarray[i+15] = myarray[i+15]+1;
      myarray[i+16] = myarray[i+16]+1;
      myarray[i+17] = myarray[i+17]+1;
      myarray[i+18] = myarray[i+18]+1;
      myarray[i+19] = myarray[i+19]+1;


      myarray[i+20] = myarray[i+20]+1;
      myarray[i+21] = myarray[i+21]+1;
      myarray[i+22] = myarray[i+22]+1;
      myarray[i+23] = myarray[i+23]+1;
      myarray[i+24] = myarray[i+24]+1;

      myarray[i+25] = myarray[i+25]+1;
      myarray[i+26] = myarray[i+26]+1;
      myarray[i+27] = myarray[i+27]+1;
      myarray[i+28] = myarray[i+28]+1;
      myarray[i+29] = myarray[i+29]+1;


      myarray[i+30] = myarray[i+30]+1;
      myarray[i+31] = myarray[i+31]+1;
      myarray[i+32] = myarray[i+32]+1;
      myarray[i+33] = myarray[i+33]+1;
      myarray[i+34] = myarray[i+34]+1;

      myarray[i+35] = myarray[i+35]+1;
      myarray[i+36] = myarray[i+36]+1;
      myarray[i+37] = myarray[i+37]+1;
      myarray[i+38] = myarray[i+38]+1;
      myarray[i+39] = myarray[i+39]+1;

    }    

    /*
    for (i = 0; i < ARRAY_SIZE; i++) {
      myarray[i] = myarray[i]+1;
    }
    */


  }
}
