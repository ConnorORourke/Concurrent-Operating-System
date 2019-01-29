//This file is used by "tests.c"
#include "ipc2.h"

void main_ipc2() {

  int count = 0;
  char* x = "world ";
  open(3,1);
  open(4,0);
  while(count < 30){
    char y[6];
    write(4,x,6);
    int z = read(3,y,6);
    if(z != 0){
      write(STDOUT_FILENO,y,6);
      count += z;
    }
  }
  kill(6,1);



  exit( EXIT_SUCCESS );
}
