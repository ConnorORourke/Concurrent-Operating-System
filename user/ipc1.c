//This file is used by "tests.c"
#include "ipc1.h"

void main_ipc1() {

  int count = 0;
  char* x = "hello ";
  int fd1 = mkfifo();
  int fd2 = mkfifo();
  open(fd1,0);
  open(fd2,1);
  while(count < 30){
    char y[6];
    write(3,x,6);
    int z = read(4,y,6);
    if(z != 0){
      write(STDOUT_FILENO,y,6);
      count += z;
    }
  }
  kill(7,1);


  exit( EXIT_SUCCESS );
}
