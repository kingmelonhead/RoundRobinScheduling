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

#define MAX 19

typedef struct {
	//stuff I think I will need based off the prompt
	float last_time;				//when last executed
	int prev_burst;					//time elapsed in previous burst
	int system_time;				//system time in ms
	int cpu_time;					//cpu time in ms
	bool is_done;					//is the user done
	bool has_ran;					//after dispatch, has user ran
} pcb; //process control block

typedef struct {
	//will add more stuff later when I know what I need
	pcb_arr = pcb[18];
} memory_container; //used to hold anything that we put in shared memory

//function definitions
int get_shm();
int get_sem();
void initialize_sems();
void initialize_shm();
void cleanup();
void early_termination_handler();





