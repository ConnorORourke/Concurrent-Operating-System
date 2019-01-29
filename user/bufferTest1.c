#include "bufferTest1.h"

void main_bufferTest1(){
  char* sent = "Hello, this definitely breaches 128 characters as is described in the buffer! Well, it doesn't yet, but it does.................NOW YAY";
  int fd = mkfifo();
  open(fd,0);
  write(fd,sent,135);

  exit(EXIT_SUCCESS);


}
