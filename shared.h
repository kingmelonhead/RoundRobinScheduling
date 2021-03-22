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

typedef struct {

	//stuff I think I will need based off the prompt
	bool wait_on_oss;
	bool is_done;					//is the user done
	bool has_ran;					//after dispatch, has user ran
	int this_pid;					//pid of process in question 
	int this_index;					//index of process in question (may not even need this idk yet)
	float last_time;				//when last executed
	int prev_burst;					//time elapsed in previous burst
	int system_time;				//system time in ms
	int cpu_time;					//cpu time in ms

} pcb; //process control block

typedef struct {

	//keeps track of current # of users and # of users done
	int user_count;
	int done_count;

	//holds the dispatched processes pid
	int scheduled_pid;

	//keeps track of various times
	unsigned int next_fork_sec;
	unsigned int next_fork_nano;
	unsigned int clock_nano;
	unsigned int clock_seconds;

	int time_quantum;   // quantum
	bool wait_flag;     // shouldnt need this in a semaphore, at least i dont think yet
						//so far only one thing will be able to mess with this value at a time anyway
	
	pcb pcb_arr[MAX];	// array of PCBs   (holds stuff regaurding the currently running processes

} memory_container; //used to hold anything that we put in shared memory

//function definitions
int get_shm();
int get_sem();
void cleanup();
void early_termination_handler();
void sem_wait();
void sem_signal();





