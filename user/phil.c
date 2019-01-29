#include "phil.h"

void main_phil(size_t argc, int* argv) {
  int eatCount = 0;
  int readFD = argv[0];
  int writeFD = argv[1];
  int philID = argv[2];

  //NOTE - This is all concerned with printing
  char firstPart[7];
  char* firstPartA = "pipe ";
  strncpy(firstPart,firstPartA,5);
  char firstPartB[2];
  itoa(firstPartB,philID+1);
  strncpy(firstPart+5,firstPartB,2);

  //NOTE - This is all concerned with printing
  char secondPart[20];
  char* secondPartA = " has eaten ";
  strncpy(secondPart,secondPartA,11);
  char* secondPartC = " times\n";



  open(readFD,1);
  open(writeFD,0);

  uint8_t x;
  while(1){

    //Thinking
    for(int i = 0; i < 2000; i++){
      asm volatile("nop");
    }
    x = 1;
    write(writeFD,&x,1);
    bool reply;
    while (read(readFD, &reply, sizeof(bool)) == 0){
      write(writeFD,&x,1);
      yield();
    }
    if(reply){
      //Eating
      for(int i = 0; i < 2000; i++){
        asm volatile("nop");
      }
      x = 2;

      //NOTE - This is concerned with printing
      write(writeFD,&x,1);
      char secondPartB[2];
      eatCount++;
      itoa(secondPartB,eatCount);
      strncpy(secondPart+11,secondPartB,2);
      strncpy(secondPart+13,secondPartC,7);
      write(STDOUT_FILENO,firstPart,8);
      write(STDOUT_FILENO,secondPart,20);
    }
  }



  exit( EXIT_SUCCESS );
}
