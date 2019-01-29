

#include "hilevel.h"
#include <stdlib.h>

//Intitialisation of various arrays, variables and macros
#define MAXPR 100
#define MAXPI 200
//Active processes - initialised to 1 due to console
int activeP = 1;
//Counter of all processes created i.e. process id upon creation
int cumP = 0;
//Counter of all file descriptors created i.e. fd upon creation
int cumFd = 0;
//Currently executing pcb index value
int executing = 0;


pipe_t pipes[MAXPI];
pcb_t pcb[MAXPR];


//Declaring relevant functions and stack spaces
extern void main_console();
extern void main_tests();
extern void testMode(ctx_t* ctx);
extern uint32_t tos_usr;
extern uint32_t tos_console;


//Function to find the largest OCCUPIED pcb index
int largestIndex(){
	for(int i = MAXPR - 1; i >= 0; i--){
		if((pcb[i].status != STATUS_EMPTY) && (pcb[i].status != STATUS_TERMINATED)){
			return i;
		}
	}
	return 0;
}

//Function to translate the pid of a process into its index in the pcb
int mapPid(int pid){
	for(int i = 0; i < MAXPR; i++){
		if(pcb[i].pid == pid){
			return pcb[i].index;
		}
	}
	return -1;
}

//Function to terminate a process
void terminateProcess(pid_t index){
	pcb[index].status     = STATUS_TERMINATED;
	activeP--;
	//Remove write/read status of process being terminated
	for(int i = 0; i < MAXPI; i++){
		if(pipes[i].reader == index){
			pipes[i].reader = 0;
		}
		if(pipes[i].writer == index){
			pipes[i].writer = 0;
		}
	}
	return;
}


//Function to find the next free pcb index
int nextFreeIndex(){
	for(int i = 0; i < MAXPR; i++){
		if(pcb[i].status == STATUS_EMPTY || pcb[i].status == STATUS_TERMINATED){
			return i;
		}
	}
	return -1;
}


//Function to find the next free pipe array index
int nextFreePipe(){
	for(int i = 0; i < MAXPI; i++){
		if(pipes[i].status == STATUS_EMPTY){
			return i;
		}
	}
	return -1;
}


//Function to select the best process based on priority and age (waiting time)
pid_t selectProcess(){
	int next = 0;
	int highest = 0;
	//NOTE - Change to 0/1 depending of whether in test mode i.e running main_tests instead of main_console
	for(int i = 0; i < MAXPR; i++){
		pcb_t proc = pcb[i];
		int totalPr = proc.priority + proc.age;
		if((totalPr > highest) && ((proc.status == STATUS_READY) || (proc.status == STATUS_EXECUTING))){
			highest = totalPr;
			next = proc.index;
		}
	}
	return next;
}

//Increasing age of non-executing processes
void increaseAge(){
  for(int i = 0; i < MAXPR; i++){
	  if(pcb[i].status == STATUS_READY){
		  pcb[i].age++;
	  }
  }
}

//Function to generate a process and add it to the pcb
pid_t generateProcess(ctx_t* ctx){
	int nextFree = nextFreeIndex();

	pcb_t* child = &pcb[nextFree];
	pcb_t* parent = &pcb[executing];

	memset(child, 0, sizeof(pcb_t));
	memcpy(&child->ctx, ctx, sizeof(ctx_t));

	child->index       = nextFree;
	child->status      = STATUS_READY;
  child->tos         = (uint32_t)(&tos_usr - ((nextFree-1) * 0x00001000));
	child->ctx.sp      = child->tos - (parent->tos - ctx->sp);
	child->ctx.gpr[0]  = 0;
	child->priority    = parent->priority;
	child->age         = 0;
	child->pid		     = cumP+1;

	memcpy((void*) (child->tos - 0x1000), (void*) (parent->tos - 0x1000), 0x1000);

	activeP++;
	cumP++;

	return cumP;
}

//Function to find a pipe's index based on its file descriptor
pipe_t* findPipe(int fd){
	for(int i = 0; i < MAXPI; i++){
		if(pipes[i].fd == fd){
			return &pipes[i];
		}
	}
	return NULL;
}

//Function to generate a pipe and allocate it space in the pipes array
int allocatePipe(){
	int nextFree = nextFreePipe();
	if(nextFree == -1){
		return -1;
	}
	pipe_t* pipe    = &pipes[nextFree];
	memset(pipe, 0, sizeof(pipe_t));

	pipe->fd 	  		= cumFd + 3;
	pipe->status    = STATUS_CREATED;
	pipe->readReady = false;
  cumFd++;
	return pipe->fd;

}


//Short term scheduler function
void stScheduler(ctx_t* ctx){
  pid_t nextProc = selectProcess();
	//No need to do anything if the process hasn't changed
	if(nextProc != executing){
		memcpy(&pcb[executing].ctx, ctx, sizeof(ctx_t)); //Preserve executing process
		if(pcb[executing].status == STATUS_EXECUTING){
			pcb[executing].status = STATUS_READY;
		}
		memcpy(ctx, &pcb[nextProc].ctx, sizeof(ctx_t)); //Restore next process
		pcb[nextProc].status = STATUS_EXECUTING;
		executing = nextProc;
		pcb[executing].age = 0;
	}
  //Increasing age of non-executing processes
  increaseAge();
  return;
}

//Function to exit a process (called by the process on itself)
void exitProcess(ctx_t* ctx){
	int exitStatus = ctx->gpr[0];
	switch (exitStatus) {
		case 0: {
			PL011_putc(UART0,'O',true);
			break;
		}
		case 1: {
			PL011_putc(UART0,'X',true);
			break;
		}
	}
	terminateProcess(executing);
	stScheduler(ctx);
}


//Function to kill a process (called by a process on another process)
void killProcess(ctx_t* ctx){
	pid_t pid = ctx->gpr[0];
	int s     = ctx->gpr[1];
	int index = mapPid(pid);
	if((index 	> 0) && (index <= largestIndex())){
		//No point terminating a process that isn't there/is already termianted
		if((pcb[index].status != 0) && (pcb[index].status != STATUS_TERMINATED)){
			terminateProcess(index);
			ctx->gpr[0] = 0;
		}
		else{
			ctx->gpr[0] = -1;
		}
	}
	return;
}


//Function to remove a pipe from the pipes array
int deallocatePipe(int fd){
	pipe_t* pipe    = findPipe(fd);
	//Only reader/writer allowed to deallocate pipe so that an unrelated proecss can't unlink a pipe
	if((pipe->reader == executing) || (pipe->writer == executing)){
		memset(pipe, 0, sizeof(pipe_t));
		return 0;
	}
	return -1;
}

//Function to open connection from a process to a given pipe
int openPipe(int fd, bool read){
	//Making sure the pipe exists
	pipe_t* pipe = findPipe(fd);
	if(pipe == NULL){
		return -1;
	}
	if(read && (pipe->reader == 0)){
		pipe->reader = executing;
		return 0;
	}
	else if(!read && (pipe->writer == 0) ){
		pipe->writer = executing;
		return 0;
	}
	return -1;
}

//Function to close a connection from a process to a given pipe
int closePipe(int fd){
	//Checking the pipe exists
	pipe_t* pipe = findPipe(fd);
	if(pipe == NULL){
		return -1;
	}
	//Only the reader/writer can close the pipe
	if((pipe->reader == executing)){
		pipe->reader = 0;
		return 0;
	}
	else if((pipe->writer == executing)){
		pipe->writer = 0;
		return 0;
	}
	return -1;

}




void hilevel_handler_rst(ctx_t* ctx){
	executing = 0;
	cumP = 0;
	activeP = 1;
	cumFd = 0;
	//Initialising pipes
	memset(pipes, 0, MAXPI*sizeof(pipe_t));

	//Assuming arrays are contiguous
	memset(pcb, 0, MAXPR * sizeof(pcb_t));

  pcb[0].index    = 0;
	pcb[0].status   = STATUS_READY;
	pcb[0].ctx.cpsr = 0x50;
	//NOTE - Change between main_console/main_tests if in test mode
	pcb[0].ctx.pc   = (uint32_t)(&main_console);
  pcb[0].tos      = (uint32_t)(&tos_console);
	pcb[0].ctx.sp   = pcb[0].tos;
	pcb[0].priority = 1;
	pcb[0].age      = 0;
	pcb[0].pid	    = cumP;

  memcpy(ctx, &pcb[0].ctx, sizeof(ctx_t));
	pcb[0].status = STATUS_EXECUTING;

  TIMER0->Timer1Load  = 0x00100000; // select period = 2^20 ticks ~= 1 sec
  TIMER0->Timer1Ctrl  = 0x00000002; // select 32-bit   timer
  TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
  TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
  TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

  GICC0->PMR          = 0x000000F0; // unmask all            interrupts
  GICD0->ISENABLER1  |= 0x00000010; // enable timer          interrupt
  GICC0->CTLR         = 0x00000001; // enable GIC interface
  GICD0->CTLR         = 0x00000001; // enable GIC distributor

  int_enable_irq();

  return;
}

void hilevel_handler_svc(ctx_t* ctx, uint32_t id){

  switch(id){
    case SYS_YIELD : {
      stScheduler(ctx);
      break;
    }
    case SYS_WRITE : {
      int   fd = (int)(ctx->gpr[0]);
      char*  x = (char*)(ctx->gpr[1]);
      int    n = (int)(ctx->gpr[2]);

			switch (fd) {
				//Can't write to stdin
				case STDIN_FILENO: {
					ctx->gpr[0] = -1;
					break;
				}
				//Writing to stdout
				case STDOUT_FILENO:
				//Writing to stderr
				case STDERR_FILENO: {
					for(int i = 0; i < n; i++){
						PL011_putc(UART0, *x++, true);
					}
					ctx->gpr[0] = n;
					break;
				}
				//Default case i.e. writing to a pipe
				default: {
					pipe_t* pipe = findPipe(fd);
					if(pipe == NULL){
						ctx->gpr[0] = -1;
						break;
					}

					if((pipe->bufferSize == 0) && (n > 0) && (n <= MAX_BUFF)){
						//Only write to buffer if not ready to read
						if(!(pipe->readReady) && (pipe->writer == executing)){
							memcpy(pipe->buffer, x, n);
							pipe->readReady = 1;
							pipe->bufferSize = n;
							ctx->gpr[0] = n;
						}
					}
					else{
						ctx->gpr[0] = 0;
					}
					break;
				}
			}
			break;
    }
    case SYS_READ : {
      int   fd = (int)(ctx->gpr[0]);
      char*  x = (char*)(ctx->gpr[1]);
      int    n = (int)(ctx->gpr[2]);

			switch (fd) {
				case STDIN_FILENO: {
					for(int i = 0; i < n; i++){
		          x[i] = PL011_getc(UART0,true);
		      }

					ctx->gpr[0] = n;
					break;
				}
				//Can't read from stdout
				case STDOUT_FILENO:
				//Can't read from stderr
				case STDERR_FILENO: {
					ctx->gpr[0] = -1;
					break;
				}
				//Default i.e. reading from a pipe
				default: {
					pipe_t* pipe = findPipe(fd);
					if(pipe == NULL){
						ctx->gpr[0] = -1;
						break;
					}
					//Only read from buffer if ready to read
					if((pipe->readReady) && (n > 0) && (n <= MAX_BUFF)){
						if(pipe->reader == executing){
							memcpy(x, pipe->buffer, n);
							pipe->readReady = 0;
							memset(pipe->buffer, 0, MAX_BUFF * sizeof(uint8_t));
							pipe->bufferSize = 0;
							ctx->gpr[0] = n;
						}
					}
					else{
						ctx->gpr[0] = 0;
					}
					break;
				}

			}
			break;
    }
		case SYS_FORK : {
			pid_t childP = generateProcess(ctx);
			//Return pid of child
			ctx->gpr[0] = childP;
			break;
		}

		case SYS_EXIT : {
			exitProcess(ctx);
			break;
		}
		case SYS_EXEC : {
			ctx->pc = ctx->gpr[0];
			size_t argc = ctx->gpr[1];
			uint8_t* argv_old = (uint8_t*) ctx->gpr[2];
			//Wiping context and resetting stack pointer
			memset(ctx->gpr, 0, 13*sizeof(uint32_t));
			//Argv is going to be at the top of the stack, starting argc bytes below
			uint8_t* argv = (uint8_t*) (pcb[executing].tos - argc + 1);
			//Copy the bytes across
			for(uint32_t i = 0; i < argc; i++){
				argv[i] = argv_old[i];
			}
			//Give function arguments, if required
			ctx->gpr[0] = argc;
			ctx->gpr[1] = (uint32_t) argv;
			//SP starts just 1 byte below argv
			ctx->sp = pcb[executing].tos - argc;
			//Clear the *rest* of the stack frame
			memset((void*) (pcb[executing].tos-0x1000), 0, 0x1000-argc);
			break;
		}
		case SYS_KILL : {
			killProcess(ctx);
			break;
		}
		case SYS_NICE : {
			//Updating priority of specified process
			pcb[mapPid(ctx->gpr[0])].priority = ctx->gpr[1];
			break;
		}
		case SYS_MKFIFO : {
			ctx->gpr[0] = allocatePipe();
			break;
		}
		case SYS_OPEN : {
			ctx->gpr[0] = openPipe(ctx->gpr[0],ctx->gpr[1]);
			break;
		}
		case SYS_CLOSE : {
			ctx->gpr[0] = closePipe(ctx->gpr[0]);
			break;
		}
		case SYS_UNLINK : {
			int fd = ctx->gpr[0];
			ctx->gpr[0] = deallocatePipe(fd);
			break;
		}
		case SYS_TEST : {
			testMode(ctx);
			ctx->gpr[0] = 0;
		}


    default   : { // 0x?? => unknown/unsupported
      break;
    }
  }

  return;
}

void hilevel_handler_irq(ctx_t* ctx){
  uint32_t id = GICC0->IAR;

  if(id == GIC_SOURCE_TIMER0){
    stScheduler(ctx); TIMER0->Timer1IntClr = 0x01;
  }
  GICC0->EOIR = id;

  return;
}


//NOTE - This function is only used by "tests.c". In order to make use of it change necessary parts of hilevel_handler_rst and stScheduler
void testMode(ctx_t* ctx){
	//Initialising pipes
	memset(pipes, 0, MAXPI*sizeof(pipe_t));

	//Initialising PCB
	//Assuming arrays are contiguous
	memset(pcb, 0, MAXPR * sizeof(pcb_t));

  pcb[0].index    = 0;
	pcb[0].status   = STATUS_READY;
	pcb[0].ctx.cpsr = 0x50;
	pcb[0].ctx.pc   = (uint32_t)(&main_tests);
  pcb[0].tos      = (uint32_t)(&tos_console);
	pcb[0].ctx.sp   = pcb[0].tos;
	pcb[0].priority = 1;
	pcb[0].age      = 0;
	pcb[0].pid	    = cumP;

  memcpy(ctx, &pcb[0].ctx, sizeof(ctx_t));
	pcb[0].status = STATUS_EXECUTING;

  TIMER0->Timer1Load  = 0x00100000; // select period = 2^20 ticks ~= 1 sec
  TIMER0->Timer1Ctrl  = 0x00000002; // select 32-bit   timer
  TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
  TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
  TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

  GICC0->PMR          = 0x000000F0; // unmask all            interrupts
  GICD0->ISENABLER1  |= 0x00000010; // enable timer          interrupt
  GICC0->CTLR         = 0x00000001; // enable GIC interface
  GICD0->CTLR         = 0x00000001; // enable GIC distributor


}
