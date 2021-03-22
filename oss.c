#include "shared.h"
//all other includes are in the header file

//needed for error handling
extern errno;

//globals
int shm_id;
int sem_id;
memory_container* shm_ptr;

//function declarations for functions not in the header file
void display_help();

int main(int argc char* argv[]) {

	//variables 
	char log_name[20];
	int max_time = 100;


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

	//initialize stuff
	initialize_sems();
	initialize_shm();


	//fork 18 children

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
	if ((shm_id = semget(key, (sizeof(pcb) * 18) + sizeof(memory_container), IPC_CREAT | 0666))) == -1) {
		perror("oss.c: shmget failed:");
		return -1;
	}
	//tries to attach shared memory
	if ((shm_ptr = (pcb*)shmat(shm_id, 0, 0)) == -1) {
		perror("oss.c: shmat failed:");
		return -1;
	}
	return 0;
}

int get_sem() {
	//creates the sysv semaphores (or tries to at least)
	key_t key = ftok("Makefile", 'a');
	//gets chared memory
	if ((sem_id = semget(key, 1, IPC_CREAT | 0666)) == -1) {
		perror("oss.c: semget failed:");
		return -1;
	}
	return 0;
}

void initialize_sems() {
	//sets the initial values in the semaphores


}

void initialize_shm() {
	//sets initial values in the shared memory

}

void cleanup() {
	shmdt(shm_ptr);
	shmctl(shm_id, IPC_RMID, NULL);	
	semctl(sem_id, 0, IPC_RMID, NULL);
}