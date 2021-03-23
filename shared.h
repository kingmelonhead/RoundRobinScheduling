#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <errno.h>
#include <time.h>

#define MAX 18
#define NUM_SEMS 1
#define QUANTUM 10;

typedef struct {

	//stuff I think I will need based off the prompt
	bool wait_on_oss;				
	bool blocked;					
	bool early_term;

	int this_pid;					//pid of process in question 
	int this_index;					//index of process in question (may not even need this idk yet)

	unsigned int prev_burst;					//time elapsed in previous burst
	unsigned int system_time;				//system time in ms
	unsigned int cpu_time;					//cpu time in ms

} pcb; //process control block

typedef struct {

	//keeps track of current # of users and # of users done
	int user_count;

	//holds the dispatched processes pid
	int scheduled_pid;
	int scheduled_index;

	//keeps track of various times
	unsigned int next_fork_sec;
	unsigned int next_fork_nano;
	unsigned int clock_nano;
	unsigned int clock_seconds;

	
	pcb pcb_arr[MAX];	// array of PCBs   (holds stuff regaurding the currently running processes

} memory_container; //used to hold anything that we put in shared memory

//function definitions
int get_shm();
int get_sem();
void cleanup();
void early_termination_handler();
void sem_wait();
void sem_signal();





