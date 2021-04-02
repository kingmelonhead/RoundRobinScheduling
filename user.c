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
FILE* file_ptr;

void death_handler();
void normalize_time();


int main(int argc, char* argv[]) {

	srand(time(NULL));


	int rand_num = rand() % 10000;

	char log_name[20];


	sprintf(log_name, "%d.log", rand_num);


	//file_ptr = fopen(log_name, "a");

	//fprintf(file_ptr, "Log opened");


	signal(SIGKILL, death_handler);
	signal(SIGINT, death_handler);
	signal(SIGTERM, death_handler);
	signal(SIGSEGV, death_handler);

	int num;
	unsigned int time;
	unsigned int burst;
	bool finished = false;

	float sec_to_ms;
	float nano_to_ms;

	//create the shared memory
	if (get_shm() == -1) {
		cleanup();
		exit(0);
	}

	//fprintf(file_ptr, "Process got shared memory\n");



	//create the semaphores
	if (get_sem() == -1) {
		cleanup();
		exit(0);
	}

	//fprintf(file_ptr, "Process got semaphores\n");

	//get the index of the process in shared memory
	

	//get the pid
	user_pid = getpid();
	

	while (1) {


		while (shm_ptr->scheduled_pid != user_pid);

		shm_ptr->scheduled_pid = 0;

		user_index = shm_ptr->scheduled_index;

		//printf("USER: PID %d woke up\n", user_pid);
		
		shm_ptr->pcb_arr[user_index].blocked = false;

		if (shm_ptr->pcb_arr[user_index].start_nano == 0 && shm_ptr->pcb_arr[user_index].start_sec == 0) {
			//determine if process will run for a little and then just die, or if it will become either a cpu or I/O task
			//based off of a random number
			srand(user_pid);
			num = rand() % (QUANTUM + 1);
			//fprintf(file_ptr, "random number to base early termination on is %d\n", num);
			if (num % 2 == 0) {
				time = rand() % 11;
				burst = rand() % ((10000000 + 1) - 1) + 1;
				//printf("PID: %d is terminating early after working for %d ms\n", user_pid, time);
				//fprintf(file_ptr, "Process is terminating early\n");
				shm_ptr->pcb_arr[user_index].early_term = true;
				shm_ptr->pcb_arr[user_index].prev_burst = burst;
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
				//fprintf(file_ptr, "Process is IO\n");
			}
			else {
				shm_ptr->pcb_arr[user_index].proc_type = CPU;
				//fprintf(file_ptr, "Process is CPU\n");
			}
		}

		//determine if an interupt will happen
		//differs by process type
		if (shm_ptr->pcb_arr[user_index].proc_type == IO) {
			//can be interrupted
			num = rand() % 13;
			if (num % 4 == 0) {
				//fprintf(file_ptr, "Process is being blocked\n");
				shm_ptr->pcb_arr[user_index].blocked = true;
				shm_ptr->pcb_arr[user_index].blocked_until_sec = shm_ptr->clock_seconds + (rand() % ((5 + 1) - 1) +1);
				shm_ptr->pcb_arr[user_index].blocked_until_nano = shm_ptr->clock_nano + (rand() % 1001);
				if (shm_ptr->pcb_arr[user_index].blocked_until_nano > 1000000000) {
					shm_ptr->pcb_arr[user_index].blocked_until_nano -= 1000000000;
					shm_ptr->pcb_arr[user_index].blocked_until_sec += 1;
				}

			}
			else {
				//fprintf(file_ptr, "Process is not being blocked\n");
				finished = true;
			}
		}
		else {
			finished = true;
		}

		burst = rand() % ((10000000 + 1) - 1) + 1;
		shm_ptr->pcb_arr[user_index].prev_burst = burst;

		shm_ptr->pcb_arr[user_index].cpu_time += (float)burst/1000000;

		if (finished) {
			sec_diff = shm_ptr->clock_seconds - shm_ptr->pcb_arr[user_index].start_sec;
			shm_ptr->pcb_arr[user_index].start_nano += shm_ptr->clock_nano;
			normalize_time();
			shm_ptr->pcb_arr[user_index].start_nano += burst;
			normalize_time();
			shm_ptr->pcb_arr[user_index].start_sec = sec_diff;
			sec_to_ms = (float)shm_ptr->pcb_arr[user_index].start_sec * 1000;
			nano_to_ms = (float)shm_ptr->pcb_arr[user_index].start_nano / 1000000;
			shm_ptr->pcb_arr[user_index].system_time = sec_to_ms + nano_to_ms;
			sem_signal(shm_id);
			break;
		}

		//signal that it is done so a new task can be scheduled
		sem_signal(shm_id);
	}
	//printf("attempting to exit user\n");
	//detatch shared memory
	cleanup();
	return 0;
}

int get_shm() {
	//creates the shared memory and attaches the pointer for it (or tries to at least)
	key_t key = ftok("Makefile", 'a');
	//gets chared memory
	if ((shm_id = shmget(key, (sizeof(pcb) * MAX) + sizeof(memory_container), IPC_EXCL)) == -1) {
		perror("user.c: shmget failed:");
		return -1;
	}
	//tries to attach shared memory
	if ((shm_ptr = (memory_container*)shmat(shm_id, 0, 0)) == (memory_container *)-1) {
		perror("user.c: shmat failed:");
		return -1;
	}
	return 0;
}

int get_sem() {
	//creates the sysv semaphores (or tries to at least)
	key_t key = ftok(".", 'a');
	//gets chared memory
	if ((sem_id = semget(key, NUM_SEMS, 0)) == -1) {
		perror("user.c: semget failed:");
		return -1;
	}
	return 0;
}

void sem_signal() {
	//used to increment semaphore
	//printf("USER: Signaling that done\n");
	struct sembuf op;
	op.sem_num = 0;
	op.sem_op = 1;
	op.sem_flg = 0;
	semop(sem_id, &op, 1);
}

void death_handler() {
	//does what name function implies
	cleanup();

}

void cleanup() {
	//does what name function implies
	shmdt(shm_ptr);
}

void normalize_time() {
	unsigned int nano = shm_ptr->pcb_arr[user_index].start_nano;
	int sec;
	if (nano >= 1000000000) {
		shm_ptr->pcb_arr[user_index].start_sec += 1;
		shm_ptr->pcb_arr[user_index].start_nano -= 1000000000;
		
	}
}