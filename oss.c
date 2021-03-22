#include "shared.h"
//all other includes are in the header file

//needed for error handling
extern errno;

//globals
int shm_id;
int sem_id;
int proc_used[MAX];
memory_container* shm_ptr;
FILE* file_ptr;
char log_buffer[200];
int ready_pids[MAX];
int blocked_queue[MAX];
int ready_in, ready_out, blocked_in, blocked_out;



//function declarations for functions not in the header file
void display_help();
void child_handler();
void fork_user(int);
void initialize_sems();
void initialize_shm();
void initialize_pcb(int);
void log_string(char *);
int get_next_location();
int check_in_ready(int);
void set_next_fork();
void spawn();
int get_user_count();

int main(int argc, char* argv[]) {

	srand(time(NULL));

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
	int temp;


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

	//open log file
	file_ptr = fopen(log_name, "a");

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
		ready_pids[i] = 0;
	}

	//initialize stuff
	initialize_sems();
	initialize_shm();

	set_next_fork();

	while (1) {

		//count the total number of users in the system
		shm_ptr->user_count = get_user_count();

		//below block is for spawning new children

		//makes a new user if the user count is less than the max alowed
		if (shm_ptr->user_count < MAX) {

			//then it will check to see if time has progressed enough for another to be spawned
			if (shm_ptr->next_fork_sec == shm_ptr->clock_seconds) {
				if (shm_ptr->next_fork_nano <= shm_ptr->clock_nano) {
					//if clock has passed then fork another user
					spawn();
					set_next_fork();
	
				}
			}
			else if (shm_ptr->next_fork_sec < shm_ptr->clock_seconds) {
				//if clock has passed then fork another user
				spawn();
				set_next_fork();
			}
		}

		//scan the blocked queue to see if the clock has passed their "blocked until" time
		//if so move back to ready at the tail (ready_in)

		//then if there is stuff in the ready queue, tell the process at "ready_out" to go
		//this is done by changing that specific pcbs wait flag to false

		//wait till it signals that its done
		sem_wait(sem_id);

		//read the stuff that the pid wrote to its pcb and log it

		//move it to blocked, or remove from ready entirely, do what is appropriate

		//increment time at end of loop to simulate the scheduler taking time

	}


	//cleanup before exit
	cleanup();
	return 0;
}

void display_help() {


}

void set_next_fork() {
	shm_ptr->next_fork_sec = (rand() % 2) + shm_ptr->clock_seconds;
	shm_ptr->next_fork_nano = (rand() % 1001) + shm_ptr->clock_seconds;
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
	shm_ptr->pcb_arr[index].this_index = index;
	sprintf(log_buffer, "Process control block at index %d has been initialized", index);
	log_string(log_buffer);
}

void initialize_shm() {
	//sets initial values in the shared memory
	int i;
	shm_ptr->wait_flag = true;
	shm_ptr->user_count = 0;
	shm_ptr->done_count = 0;
	shm_ptr->time_quantum = 10;   //time in ms
	shm_ptr->clock_nano = 0;
	shm_ptr->clock_seconds = 0;
	shm_ptr->scheduled_pid = 0;
	for (i = 0; i < MAX; i++) {
		initialize_pcb(i);
	}
}

int get_next_location() {
	//gets next free spot in pid table
	int i;
	for (i = 0; i < MAX; i++) {
		if (proc_used[i] == 0) {
			return i;
		}
	}
	return -1;
}

int get_user_count() {
	int total = 0;
	int i;
	for (i = 0; i < MAX; i++) {
		if (ready_pids[i] != 0) {
			total++;
		}
	}
	for (i = 0; i < MAX; i++) {
		if (blocked_queue[i] != 0) {
			total++;
		}
	}
	return total;
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

void spawn() {
	int temp;
	temp = get_next_location();
	fork_user(temp);
	proc_used[temp] = 1;
	shm_ptr->user_count++;
}

int check_in_ready(int id) {

}

void cleanup() {
	shmdt(shm_ptr);
	shmctl(shm_id, IPC_RMID, NULL);	
	semctl(sem_id, 0, IPC_RMID, NULL);
	fclose(file_ptr);
}

void early_termination_handler() {
	cleanup();
	exit(0);
}

void child_handler() {
	//function for handling when a child dies
	pid_t pid;
	while ((pid = waitpid((pid_t)(-1), 0, WNOHANG)) > 0) {
		//do stuff when child dies
	}

}