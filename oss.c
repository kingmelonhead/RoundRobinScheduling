#include "shared.h"
//all other includes are in the header file

//needed for error handling
extern errno;

//globals
int shm_id;
int sem_id;
int proc_used[19];
memory_container* shm_ptr;
FILE* file_ptr;
char log_buffer[200];



//function declarations for functions not in the header file
void display_help();
void child_handler();
void fork_user(int);
void initialize_sems();
void initialize_shm();
void initialize_pcb(int);
void log_string(char *);


int main(int argc, char* argv[]) {

	//signal handlers
	signal(SIGINT, early_termination_handler);
	signal(SIGKILL, early_termination_handler);
	signal(SIGALRM, early_termination_handler);

	//set up for the ctrl_c handler
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = child_handler;
	sigaction(SIGCHLD, &sa, NULL);

	//variables 
	char log_name[20];
	int max_time = 100;
	int opt;
	int i;


	// gets options set up
	while ((opt = getopt(argc, argv, "hs:l:")) != -1) {
		switch (opt) {
		case 'h':
			display_help();
			break;
		case 's':
			max_time = atoi(optarg);
			break;
		case 'l':
			if (strlen(optarg) > 18) {
				printf("logfile argument too long, defaulting to 'logfile'\n\n");
			}
			else {
				strcpy(log_name, optarg);
			}
			break;
		default:
			printf("Error: Invaliid option, calling help menu and exiting early...\n\n");
			display_help();
			exit(0);
		}
	}

	//sets the alarm for the timeout based on the time provide
	//or if not provided, the default of 100 seconds
	alarm(max_time);

	//this happens if the user doesnt want to set their own logfile name. Defaults to 'logfile'
	if (strcmp(log_name, "")) {
		strcpy(log_name, "logfile");
	}

	//create the shared memory
	if (get_shm() == -1) {
		cleanup();
		exit(0);
	}

	//create the semaphores
	if (get_sem() == -1) {
		cleanup();
		exit(0);
	}

	//initialize table for tracking which index PCBs are beeing occupied by children
	for (i = 0; i < MAX; i++) {
		proc_used[i] = 0;
	}

	//initialize stuff
	initialize_sems();
	initialize_shm();


	//fork 19 children
	for (i = 0; i < MAX; i++) {
		fork_user(i);
	}
	

	//after forking is done then start with the scheduling


	//cleanup before exit
	cleanup();
	return 0;
}

void display_help() {


}

int get_shm() {
	//creates the shared memory and attaches the pointer for it (or tries to at least)
	key_t key = ftok("README.md", 'a');
	//gets chared memory
	if ((shm_id = semget(key, (sizeof(pcb) * MAX) + sizeof(memory_container), IPC_CREAT | 0666)) == -1) {
		perror("oss.c: shmget failed:");
		return -1;
	}
	//tries to attach shared memory
	if ((shm_ptr = (memory_container *)shmat(shm_id, 0, 0)) == (void*)-1) {
		perror("oss.c: shmat failed:");
		return -1;
	}
	return 0;
}

int get_sem() {
	//creates the sysv semaphores (or tries to at least)
	key_t key = ftok("Makefile", 'a');
	//gets chared memory
	if ((sem_id = semget(key, NUM_SEMS, IPC_CREAT | 0666)) == -1) {
		perror("oss.c: semget failed:");
		return -1;
	}
	return 0;
}

void sem_wait() {
	//used to wait and decrement semaphore
	struct sembuf op;
	op.sem_num = 0;
	op.sem_op = -1;
	op.sem_flg = 0;
	semop(sem_id, &op, 1);
}

void log_string(char* string) {

}


void initialize_sems() {
	//sets the initial values in the semaphores
	semctl(sem_id, 0, SETVAL, 1);
}

void initialize_pcb(int index) {
	shm_ptr->pcb_arr[index].is_done = false;
	shm_ptr->pcb_arr[index].cpu_time = false;
	shm_ptr->pcb_arr[index].cpu_time = 0;
	shm_ptr->pcb_arr[index].system_time = 0;
	shm_ptr->pcb_arr[index].prev_burst = 0;
	shm_ptr->pcb_arr[index].last_time = (float)shm_ptr->clock_seconds + ((float)shm_ptr->clock_nano / 1000);

	sprintf(log_buffer, "Process control block at index %d has been initialized", index);
	log_string(log_buffer);
}

void initialize_shm() {
	//sets initial values in the shared memory
	int i;
	shm_ptr->wait_flag = true;
	shm_ptr->total_wait_time = 0;
	shm_ptr->user_count = 0;
	shm_ptr->done_count = 0;
	shm_ptr->time_quantum = 100;
	shm_ptr->clock_nano = 0;
	shm_ptr->clock_seconds = 0;
	shm_ptr->scheduled_pid = 0;
	for (i = 0; i < MAX; i++) {
		initialize_pcb(i);
	}
}

void fork_user(int index) {
	//forks user at index 

	if ((shm_ptr->pcb_arr[index].this_pid = fork()) == -1) {
		perror("oss.c: Failed forking child...");
		cleanup();
		exit(1);
	}
	else {
		proc_used[index] = 1;
		if (shm_ptr->pcb_arr[index].this_pid == 0) {
			execl("./user", "./user", (char *)0);
		}
	}
}

void cleanup() {
	shmdt(shm_ptr);
	shmctl(shm_id, IPC_RMID, NULL);	
	semctl(sem_id, 0, IPC_RMID, NULL);
}

void early_termination_handler() {
	cleanup();
	exit(0);
}

void child_handler() {


}