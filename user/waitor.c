#include "waitor.h"

extern void main_phil();

int forks[16];

//Can't uses memset as it uses byte sizes
void initialiseForks(){
  for(int i = 0; i < 16; i++){
    forks[i] = 1;
  }
}

//Function to check if forks are available, and update array if so
bool forkFetch(int i){
  if(forks[i] == 1 && forks[(i+1) % 16] == 1){
    forks[i] = 0;
    forks[(i+1) % 16] = 0;
    return true;
  }
  return false;
}

//Function to restore forks
void forkReturn(int i){
  forks[i] = 1;
  forks[(i+1) % 16] =  1;
  return;
}

void main_waitor(){
    int fd[32];
    for(int i = 0; i < 16; i++){
      int in = mkfifo();
      open(in,1);
      int out = mkfifo();
      open(out,0);
      fd[2*i] = in;
      fd[(2*i) + 1] = out;

      pid_t pid = fork();
      if(pid == 0){
        int fds[3] = {out,in,i};
        exec( &main_phil ,sizeof(int)*3 , fds);
      }
    }
    initialiseForks();
    while(1){
      for(int i = 0; i < 16; i++){

        uint8_t x;
        int y = read(fd[i*2],&x,1);
        if(y > 0){
          if(x == 1){
            bool available = forkFetch(i);
            if(available){
              write(fd[(i*2)+1],&x,1);
            }
          }
          else if(x == 2){
            forkReturn(i);
          }
        }
      }
    }
    exit( EXIT_SUCCESS );
}
