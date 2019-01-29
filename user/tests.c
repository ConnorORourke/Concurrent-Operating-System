#include <tests.h>

extern void main_P5();
extern void main_P3test();
extern void main_P4test();
extern void main_killer();

extern void main_ipc1();
extern void main_ipc2();

extern void main_bufferTest1();
extern void main_bufferTest2();

extern void main_waitor();

int testNo = 1;

uint32_t test1[2] = {(uint32_t)&main_P3test,(uint32_t)&main_P4test};
uint32_t test2[2] = {(uint32_t)&main_P3test,(uint32_t)&main_killer};
uint32_t test3[2] = {(uint32_t)&main_ipc1,(uint32_t)&main_ipc2};
uint32_t test4[2] = {(uint32_t)&main_bufferTest1,(uint32_t)&main_bufferTest2};

extern void lolevel_handler_rst();

void main_tests(){

  switch (testNo) {
    //Test 1 - Run P5. Testing Fork,Exec and Exit
    case 1:{
      write(STDOUT_FILENO,"\nTest 1:\n",9);
      pid_t pid = fork();
      if(pid == 0){
        exec(&main_P5,0,NULL);
      }
      yield();
      break;
    }
    //Test 2 - Run P3 and P4. Testing the scheduler works
    case 2:{
      write(STDOUT_FILENO,"\nTest 2:\n",9);
      for(int i = 0; i < 2; i++){
        pid_t pid = fork();
        if(pid == 0){
          exec((const void *) test1[i],0,NULL);
        }
      }
      yield();
      break;
    }
    //Test 3 - Run 2 processes, have one terminate the other
    case 3:{
      write(STDOUT_FILENO,"\nTest 3:\n",9);
      for(int i = 0; i < 2; i++){
        pid_t pid = fork();
        if(pid == 0){
          exec((const void *) test2[i],0,NULL);
        }
      }
      yield();
      break;

    }
    //Test 4 - Run 2 processes, have them constantly writing and reading "hello" and "world", and printing them out
    case 4:{
      write(STDOUT_FILENO,"\nTest 4:\n",9);
      for(int i = 0; i < 2; i++){
        pid_t pid = fork();
        if(pid == 0){
          exec((const void *) test3[i],0,NULL);
        }
      }
      yield();
      break;


    }
    //Test 5 - Sending a string larger than the buffer size between two 2 processes
    case 5:{
      write(STDOUT_FILENO,"\nTest 5:\n",9);
      for(int i = 0; i < 2; i++){
        pid_t pid = fork();
        if(pid == 0){
          exec((const void *) test2[i],0,NULL);
        }
      }
      yield();
      break;
    }
    default:{
      write(STDOUT_FILENO,"\nAll tests finished! Leaving you with dining philosophers...\n",61);
      pid_t pid = fork();
      if(pid == 0){
        exec(&main_waitor,0,NULL);
      }
      yield();
      break;
      exit(EXIT_SUCCESS);
    }

  }
  testNo++;
  test();
}
