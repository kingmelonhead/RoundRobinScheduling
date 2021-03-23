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
int temp_pid;



//function declarations for functions not in the header file
void display_help();
void child_handler();
void fork_user(int);
void initialize_sems();
void initialize_shm();
void initialize_pcb(int);
void log_string(char *);
int get_next_location();
void purge_blocked();
void set_next_fork();
void spawn();
int get_user_count();
int get_index_by_pid(int);
void normalize_clock();
void normalize_fork();
void kill_pids();
int count_ready();


int main(int argc, char* argv[]) {

	//initialzie global variables of interest in the main

	ready_in = 0;
	ready_out = 0;

	blocked_in = 0;
	blocked_out = 0;

	srand(time(NULL));

	//signal handlers

	signal(SIGKILL, early_termination_handler);
	signal(SIGALRM, early_termination_handler);
	signal(SIGINT, early_termination_handler);
	signal(SIGSEGV, early_termination_handler);


	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = child_handler;
	sigaction(SIGCHLD, &sa, NULL);

	//variables 
	char log_name[25];
	int max_time = 100;
	int opt;
	int i;
	int temp, ready;
	unsigned int nano_change;
	int rand_time;
	char default_log[20];
	strcpy(default_log, "logfile");
	int new_log_name = 0;
	
	

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
			if (strlen(optarg) > 19) {
				printf("logfile argument too long, defaulting to 'logfile'\n\n");
			}
			else {
				strcpy(log_name, optarg);
				new_log_name = 1;
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
	//alarm(max_time);

	//this happens if the user doesnt want to set their own logfile name. Defaults to 'logfile'
	if (new_log_name == 0) {
		strcpy(log_name, default_log);
	}

	

	//open log file
	file_ptr = fopen(log_name, "a");

	fprintf(file_ptr, "log opened by oss.c\n");


	//create the semaphores
	if (get_sem() == -1) {
		cleanup();
		exit(0);
	}

	//create the shared memory
	if (get_shm() == -1) {
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

	
	int user_count_debug;
	int ready_count;
	int temp_pid;
	while (1) {

		sleep(2);

		ready_count = count_ready();

		//count the total number of users in the system
		shm_ptr->user_count = get_user_count();

		user_count_debug = shm_ptr->user_count;

		printf("user count: %d, \n", user_count_debug);
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

		purge_blocked();


		//then if there is stuff in the ready queue, tell the process at "ready_out" to go
		//this is done by changing that specific pcbs wait flag to false

		//if structure will eveluate to true in the case that there are things in the ready queue
		if (ready_count > 0) {
			printf("things are ready\n");
			
			temp_pid = ready_pids[ready_out];
			temp = get_index_by_pid(temp_pid);
			printf("setting scheduled index to %d\n", temp);
			shm_ptr->scheduled_index = temp;

			//rand()%((nMax+1)-nMin) + nMin -- from stack overflow for reference becasue I always forget this
			//progress time before dispatch
			rand_time = rand() % ((10000 + 1) - 100) + 100;
			shm_ptr->clock_nano += rand_time;
			normalize_clock();

			//dispatch
			
			
			printf("waking up PID %d at index %d of the ready queue, index %d in shm \n", temp_pid, ready_out, temp);
			//this will cut the process loose
			shm_ptr->scheduled_pid = temp_pid;
			
			ready_pids[ready_out] = 0;
			ready_out = (ready_out + 1) % MAX;

			//wait till it signals that its done
			sem_wait(sem_id);
			printf("recieved signal that done\n");

			//interpret what the result of the pcb means, log accordingly
			if (shm_ptr->pcb_arr[temp].early_term) {
				//if it terminated early
				sprintf(log_buffer, "OSS.C: PID: %d has terminated early after spending %.4f ms in the cpu/system\n", temp_pid, shm_ptr->pcb_arr[temp].cpu_time);
				log_string(log_buffer);
				proc_used[temp] = 0;
			}
			else if (shm_ptr->pcb_arr[temp].blocked) {
				//if it got blocked
				blocked_queue[blocked_in] = temp_pid;
				blocked_in = (blocked_in + 1) % MAX;
				sprintf(log_buffer, "OSS.C: PID: %d has been blocked and is going into blocked queue. Last Burst: %d ns\n", temp_pid, shm_ptr->pcb_arr[temp].prev_burst);
				log_string(log_buffer); 
			}
			else {
				//only other condition is that it completed before being interupted
				proc_used[temp] = 0;
				sprintf(log_buffer, "OSS.C: PID: %d has finished. CPU time: %.2f ms System time: %.2f ms Last burst: %d ns\n", temp_pid, shm_ptr->pcb_arr[temp].cpu_time, shm_ptr->pcb_arr[temp].system_time, shm_ptr->pcb_arr[temp].prev_burst);
				log_string(log_buffer);
			}

			
		}


		//increment time at end of loop to simulate the scheduler taking time
		shm_ptr->clock_seconds += 1;
		nano_change = rand() % 1001;
		shm_ptr->clock_nano += nano_change;
		normalize_clock();

		sprintf(log_buffer, "CLOCK: %d : %d \n", shm_ptr->clock_seconds, shm_ptr->clock_nano);
		log_string(log_buffer);
	}

	printf("exited qhile loop, about to cleanup\n");

	//cleanup before exit
	cleanup();
	return 0;
}

int count_ready() {
	int count = 0;
	int i;
	for (i = 0; i < MAX; i++) {
		if (ready_pids[i] > 0) {
			count++;
		}
	}
	return count;
}

void display_help() {


}

void normalize_clock() {
	unsigned int nano = shm_ptr->clock_nano;
	int sec;
	if (nano >= 1000000000) {
		shm_ptr->clock_seconds += 1;
		shm_ptr->clock_nano -= 1000000000;
	}
}

void normalize_fork() {
	unsigned int nano = shm_ptr->next_fork_nano;
	int sec;
	if (nano >= 1000000000) {
		shm_ptr->next_fork_sec += 1;
		shm_ptr->next_fork_nano -= 1000000000;
	}
}


void set_next_fork() {
	shm_ptr->next_fork_sec = (rand() % 2) + shm_ptr->clock_seconds;
	shm_ptr->next_fork_nano = (rand() % 1001) + shm_ptr->clock_nano;
	normalize_fork();
}

int get_shm() {
	//creates the shared memory and attaches the pointer for it (or tries to at least)
	key_t key = ftok("Makefile", 'a');
	//gets chared memory
	if ((shm_id = shmget(key, (sizeof(pcb) * MAX) + sizeof(memory_container), IPC_CREAT | 0666)) == -1) {
		perror("oss.c: shmget failed:");
		return -1;
	}
	//tries to attach shared memory

	
	if ((shm_ptr = (memory_container *)shmat(shm_id, 0, 0)) == (memory_container *)-1) {
		perror("oss.c: shmat failed:");
		return -1;
	}

	return 0;
}

int get_sem() {
	//creates the sysv semaphores (or tries to at least)
	key_t key = ftok(".", 'a');
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

	//log the passed string to the log file
	fputs(string, file_ptr);

}


void initialize_sems() {
	//sets the initial values in the semaphores
	semctl(sem_id, 0, SETVAL, 0);
}

void initialize_pcb(int index) {

	shm_ptr->pcb_arr[index].blocked = false;
	shm_ptr->pcb_arr[index].early_term = false;
	shm_ptr->pcb_arr[index].wait_on_oss = true;
	shm_ptr->pcb_arr[index].start_nano = 0;
	shm_ptr->pcb_arr[index].start_sec = 0;
	shm_ptr->pcb_arr[index].cpu_time = 0;
	shm_ptr->pcb_arr[index].system_time = 0;
	shm_ptr->pcb_arr[index].prev_burst = 0;
	shm_ptr->pcb_arr[index].this_index = index;

}

void initialize_shm() {
	//sets initial values in the shared memory
	int i;
	shm_ptr->user_count = 0;   
	shm_ptr->clock_nano = 0;
	shm_ptr->clock_seconds = 0;
	shm_ptr->scheduled_pid = 0;
	shm_ptr->scheduled_index = -1;

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

	int pid;

	if ((pid = fork()) == -1) {
		perror("oss.c: Failed forking child...");
		cleanup();
		exit(1);
	}
	else {
		if (pid != 0) {
			shm_ptr->pcb_arr[index].this_pid = pid;
			proc_used[index] = 1;
			initialize_pcb(index);
			printf("PID: %d has been forked\n", pid);
			ready_pids[ready_in] = shm_ptr->pcb_arr[index].this_pid;
			printf("PID: %d was put in the ready queue at index %d\n", shm_ptr->pcb_arr[index].this_pid, ready_in);
			ready_in = (ready_in + 1) % MAX;
			printf("ready in incremented\n");
		}
		else {
			execl("./user", "./user", (char *)0);
		}
	}
}

void spawn() {
	int temp;
	temp = get_next_location();
	fork_user(temp);
}

int get_index_by_pid(int pid) {
	int i;
	for (i = 0; i < MAX; i++) {
		if (shm_ptr->pcb_arr[i].this_pid == pid) {
			return shm_ptr->pcb_arr[i].this_index;
		}
	}
	return -1;
}

void purge_blocked() {

	int pid = blocked_queue[blocked_out];
	int i = get_index_by_pid(pid);
	if (pid != 0) {
		if (shm_ptr->pcb_arr[i].blocked_until_sec == shm_ptr->clock_seconds) {
			if (shm_ptr->pcb_arr[i].blocked_until_nano <= shm_ptr->clock_nano) {
				//move to ready
				blocked_queue[blocked_out] = 0;
				blocked_out = (blocked_out + 1) % MAX;
				ready_pids[ready_in] = pid;
				printf("ready_in incremented in purge blocked()\n");
				ready_in = (ready_in + 1) % MAX;

			}
		}
		else if (shm_ptr->next_fork_sec < shm_ptr->clock_seconds) {
			//move to ready

			blocked_queue[blocked_out] = 0;
			blocked_out = (blocked_out + 1) % MAX;
			ready_pids[ready_in] = pid;
			ready_in = (ready_in + 1) % MAX;

		}
	}
}

void cleanup() {
	kill_pids();
	sleep(1);
	shmdt(shm_ptr);
	shmctl(shm_id, IPC_RMID, NULL);	
	semctl(sem_id, 0, IPC_RMID, NULL);
	fclose(file_ptr);
}

void kill_pids() {
	int i;
	for (i = 0; i < MAX; i++) {
		if (blocked_queue[i] != 0) {
			kill(blocked_queue[i], SIGKILL);
		}
	}
	for (i = 0; i < MAX; i++) {
		if (ready_pids[i] != 0) {
			kill(ready_pids[i], SIGKILL);
		}
	}
}

void early_termination_handler() {
	cleanup();
	sleep(2);
	exit(0);
}

void child_handler() {
	//function for handling when a child dies
	pid_t pid;
	while ((pid = waitpid((pid_t)(-1), 0, WNOHANG)) > 0) {
		//do stuff when child dies
	}

}