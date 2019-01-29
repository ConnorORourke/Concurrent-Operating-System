//This file is used by "tests.c"
#include "bufferTest2.h"

void main_bufferTest2(){
  open(5,1);
  char receive[135];
  read(5,receive,135);
  write(STDOUT_FILENO,receive,135);

  exit(EXIT_SUCCESS);


}
