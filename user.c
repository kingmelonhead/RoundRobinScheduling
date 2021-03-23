#include "shared.h"
//other includes are in the header file

//needed for error handling
extern errno;

//global vars
int shm_id, sem_id;
memory_container* shm_ptr;
int user_index;
int user_pid;
int sec_diff;

void death_handler();
void normalize_time();


int main(int argc, char* argv[]) {


	signal(SIGKILL, death_handler);
	signal(SIGINT, death_handler);
	signal(SIGTERM, death_handler);

	int num;
	unsigned int time;

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

	//get the pid
	user_pid = shm_ptr->scheduled_pid;
	
	//get the index of the process in shared memory
	user_index = shm_ptr->scheduled_index;

	while (shm_ptr->pcb_arr[user_index].wait_on_oss);

	if (shm_ptr->pcb_arr[user_index].start_nano == 0 && shm_ptr->pcb_arr[user_index].start_sec == 0) {
		//determine if process will run for a little and then just die, or if it will become either a cpu or I/O task
		//based off of a random number
		srand(user_pid);
		num = rand() % (QUANTUM + 1);
		if (num % 2 == 0) {
			time = rand() % 11;
			printf("PID: %d is terminating early after working for %d ms\n", user_pid, time);
			shm_ptr->pcb_arr[user_index].early_term = true;
			shm_ptr->pcb_arr[user_index].prev_burst = time;
			shm_ptr->pcb_arr[user_index].cpu_time += time;
			shm_ptr->pcb_arr[user_index].system_time += time;
			sem_signal(shm_id);
			cleanup();
			exit(0);
		}
		//if the thing doesnt end pre maturely then clock at dispatch time is bookmarked.
		//used for ellapsed time calculation later on
		shm_ptr->pcb_arr[user_index].start_nano = shm_ptr->clock_nano;
		shm_ptr->pcb_arr[user_index].start_sec = shm_ptr->clock_seconds;

		//type of process is randomized (wether its i/o or cpu)
		num = rand() % (100 + 1);
		if (num % 3 == 0) {
			shm_ptr->pcb_arr[user_index].proc_type = IO;
		}
		else {
			shm_ptr->pcb_arr[user_index].proc_type = CPU;
		}
	}
	
	//determine if an interupt will happen
	//differs by process type
	if (shm_ptr->pcb_arr[user_index].proc_type == IO) {
		//can be interrupted
	}
	else {
		//less likely to be interrupted
		//for testing this wont be able to be interrupted 
	}


	//calculate times accordingly and write to pcb
	//these times will be used for calculations and incrementations in oss.c

	//signal that it is done so a new task can be scheduled
	sem_signal(shm_id);

	//detatch shared memory
	cleanup();
}

int get_shm() {
	//creates the shared memory and attaches the pointer for it (or tries to at least)
	key_t key = ftok("README.md", 'a');
	//gets chared memory
	if ((shm_id = semget(key, (sizeof(pcb) * MAX) + sizeof(memory_container), IPC_EXCL)) == -1) {
		perror("user.c: shmget failed:");
		return -1;
	}
	//tries to attach shared memory
	if ((shm_ptr = (memory_container*)shmat(shm_id, 0, 0)) == (void*)-1) {
		perror("user.c: shmat failed:");
		return -1;
	}
	return 0;
}

int get_sem() {
	//creates the sysv semaphores (or tries to at least)
	key_t key = ftok("Makefile", 'a');
	//gets chared memory
	if ((sem_id = semget(key, NUM_SEMS, 0)) == -1) {
		perror("user.c: semget failed:");
		return -1;
	}
	return 0;
}

void sem_signal() {
	//used to increment semaphore

	struct sembuf op;
	op.sem_num = 0;
	op.sem_op = 1;
	op.sem_flg = 0;
	semop(sem_id, &op, 1);
}

void death_handler() {
	cleanup();
}

void cleanup() {
	shmdt(shm_ptr);
}

void normalize_time() {
	unsigned int nano = shm_ptr->pcb_arr[user_index].start_nano;
	int sec;
	while (nano >= 1000000000) {
		shm_ptr->pcb_arr[user_index].start_sec += 1;
		shm_ptr->pcb_arr[user_index].start_nano -= 1000000000;
		nano -= 1000000000;
	}
}