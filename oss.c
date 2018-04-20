#include "proj5.h"

int pcbid;	// Shared memory ID of process control block table
int clkid;	// Shared memory ID of simulated system clock
int msgid;	// Message queue ID
int semid;	// Semaphore ID
int descid;	// Resource description ID
FILE *fp;	// Log file

message_buf buf;	// Message content
size_t buf_length;	// Message length

void outputTable(pid_t[], descriptor[]);
void pidTable(pid_t[]);
void allocate(int, pid_t);
void block(pid_t);
void unblock(pid_t);


void handler(int signo) {
	printf("OSS: Terminating by signal\n");
	shmctl(clkid, IPC_RMID, NULL);	// Release simulated system clock memory
	shmctl(descid, IPC_RMID, NULL);	// Release resource description memory
	msgctl(msgid, IPC_RMID, NULL);	// Release message queue
	fclose(fp);
	exit(1);
}

int main(int argc, char *argv[]) {
	int p, r;
	int opt, vflag;
	int currentUsers = 0;
	pid_t Proc[MAXUSERS];

	srand(time(NULL));

	// Parse command line options:
	while ((opt = getopt(argc, argv, "v")) != -1) {
		switch (opt) {
			case 'v':
				// Run in verbose mode
				vflag = 1;
				break;
			default:
				vflag = 0;
		}
	}

	signal(SIGINT, handler);	// Catch interupt signal
	signal(SIGALRM, handler);	// Catch alarm signal
	alarm(5);			// Send alarm signal

	// Open log file for output
	fp = fopen("data.log", "w");
	if (fp == NULL) {
		fprintf(stderr, "Can't open file\n");
		raise(SIGINT);
	}

	// Initialize child process table
	for (p = 0; p < MAXUSERS; p++) {
		Proc[p] = 0;
	}

	// Initialize resource descriptors in shared memory
	if ((descid = shmget(DESCKEY, SIZE*sizeof(descriptor), IPC_CREAT | 0666)) < 0) {
		perror("oss: shmget");
		raise(SIGINT);
	}
	descriptor *Res = shmat(descid, NULL, 0);
	for (r = 0; r < SIZE; r++) {
		// Generate random number (1-10) of instances
		int inst = (rand() % 10) + 1;
		Res[r].R = inst;
		Res[r].V = inst;
		for (p = 0; p < MAXUSERS; p++) {
			Res[r].C[p] = 0;
			Res[r].A[p] = 0;
			Res[r].CA[p] = 0;
		}
	}

	// Initialize clock in shared memory
	if ((clkid = shmget(CLKKEY, sizeof(sim_time), IPC_CREAT | 0666)) < 0 ) {
		perror("oss: shmget");
		raise(SIGINT);
	}
	sim_time *simClock = shmat(clkid, NULL, 0);
	simClock->sec = 0;
	simClock->nano = 0;

	// Create message queue
	if ((msgid = msgget(MSGKEY, IPC_CREAT | 0666)) < 0) {
		perror("oss: msgget");
		raise(SIGINT);
	}

	// Lets try to out put it
	if (vflag)
		outputTable(Proc, Res);

//-------------------

	while(1) {

		/*
		// Fork children if max has not been reached
		if (currentUsers < MAXUSERS) {
			pid = fork();
			switch (pid) {
				case -1 :
					printf("Could not spawn child\n");
					break;
				case 0 :
					execl("./user", "user", NULL);
					fprintf(stderr, "Fail to execute child process\n");
					exit(1);
				default :
					// Find empty slot in pid table
					for (procNum = 0; procNum < MAXUSERS; procNum++)
						if (P[procNum] == 0)
							break;
					P[procNum] = pid;
					totalUsers++;
					currentUsers++;
					printf("Creating P%d (PID: %ld)\n", procNum, pid);
					pidTable(P);

			}
		}
		*/

		// Handle messages from children
		if (msgrcv(msgid, &buf, MSGSZ, (long)getpid(), IPC_NOWAIT) != (ssize_t)-1) {

			// Parse message
			char *ptr;
			pid_t userPid = (pid_t)strtol(buf.mtext, &ptr, 10);
			int msgType = (int)strtol(ptr, &ptr, 10);
			r = (int)strtol(ptr, &ptr, 10);
			printf("Message type %d recieved from %ld regarding %ld\n", msgType, userPid, r);

			// Determine which process sent the message
			for (p = 0; p < MAXUSERS; p++)
				if (Proc[p] == userPid)
					break;
			if (p == 18) {
				fprintf(stderr, "oss: no such pid in table\n");
				exit(1);
			}

			// Handle message
			switch (msgType) {
				case TERM:
					waitpid(Proc[p], NULL, 0);
					Proc[p] = 0;
					pidTable(Proc);
					currentUsers--;
					break;
				case REQ:
					fprintf(fp, "Master has detected Process P%d requesting R%d at time %d:%09d\n", p, r, simClock->sec, simClock->nano);
					allocate(Proc[p], r);
					break;
				case REL:
					Res[r].V++;
					Res[r].C[p]--;
					Res[r].A[p]--;
					break;

			}
			// Respond to user


		}
	}


//-------------------


	// Cleanup
	shmctl(clkid, IPC_RMID, NULL);	// Release process control block memeory
	shmctl(descid, IPC_RMID, NULL);	// Release resource description memory
	msgctl(msgid, IPC_RMID, NULL);  // Release message queue memory
	fclose(fp);

	exit(0);
}

// Infinitely repeated loop
/*
	while(totalUsers < 30) {

		// Fork children if max has not been reached
		if (currentUsers < MAXUSERS) {
			pid = fork();
			switch (pid) {
				case -1 :
					printf("Could not spawn child\n");
					break;
				case 0 :
					execl("./user", "user", NULL);
					fprintf(stderr, "Fail to execute child process\n");
					exit(1);
				default :
					// Find empty slot in pid table
					for (procNum = 0; procNum < MAXUSERS; procNum++)
						if (P[procNum] == 0)
							break;
					P[procNum] = pid;
					totalUsers++;
					currentUsers++;
					printf("Creating P%d (PID: %ld)\n", procNum, pid);
					pidTable(P);

			}
		}

		// Handle messages from children
		if (msgrcv(msgid, &buf, MSGSZ, (long)getpid(), IPC_NOWAIT) != (ssize_t)-1) {

			// Parse message
			char *ptr;
			pid_t userPid = (pid_t)strtol(buf.mtext, &ptr, 10);
			int msgType = (int)strtol(ptr, &ptr, 10);
			int msgData = (int)strtol(ptr, &ptr, 10);
			printf("Message type %d recieved from %ld regarding %ld\n", msgType, userPid, msgData);

			// Determine which process sent the message
			for (procNum = 0; procNum < MAXUSERS; procNum++)
				if (P[procNum] == userPid)
					break;

			// Handle message
			switch (msgType) {
				case TERM:
					//printf("P%d is terminating\n", procNum);
					waitpid(P[procNum], NULL, 0);
					P[procNum] = 0;
					pidTable(P);
					currentUsers--;
					break;
				case REQ:
					fprintf(fp, "Master has detected Process P%d requesting R%d at time %d:%09d\n", procNum, msgData, simClock->sec, simClock->nano);
					allocate(P[procNum], msgData);
					break;
				case REL:
					fprintf(fp, "Master has detected Process P%d releasing R%d at time %d:%09d\n", procNum, msgData, simClock->sec, simClock->nano);
					break;

			}
			// Respond to user


		}
	}
*/
/*
	// Kill remaining user processes
	for (p = 0; p < MAXUSERS; p++)
		if (P[p] != 0) {
			kill(P[p], SIGKILL);
			waitpid(P[p], NULL, 0);
		}
*/
	// Release memeory

//	exit(0);
//}


// Write formated table to log file
void outputTable(pid_t P[], descriptor R[]) {
	int p, r;
	for (p = -1; p < MAXUSERS; p++) {

		if (p < 0) {
			fprintf(fp, "\t");
			for (r = 0; r < SIZE; r++) {
				fprintf(fp, "R%d\t", r);
			}
		}
		else {
			fprintf(fp, "P%d\t", p);
			for (r = 0; r < SIZE; r++) {	
				fprintf(fp, "%d\t", R[r].A[p]);
			}
		}
	
		fprintf(fp, "\n");
	}
}

// Run banker's algorithm
int banker(descriptor R[], r, p) {
	int i, j;
	int safe = 0;	// 0 if not in a safe state
	int canrun = 0;	// 0 if no process can run

	// Update C-A table
	for (i = 0; i < MAXUSERS; i++) {
		for (j = 0; j < SIZE; j++) {
			R[i].CA[j] = R[i].C[j] - R[i].A[j]
		}
	}

	// Check if the system is in a safe state, but finding a process that can run
	for (i = 0; i < MAXUSERS; i++) {
		for (j = 0; j < SIZE; j++) {
			
		}
	}

}

// Allocate resource
void allocate(int resource, pid_t userPid) {
	buf.mtype = (long)userPid;
	sprintf(buf.mtext, "%ld %d %d", (long)getpid(), ALC, resource);
	buf_length = strlen(buf.mtext) + 1;
	if (msgsnd(msgid, &buf, buf_length, 0) < 0) {
		perror("oss: msgsnd");
		exit(1);
	}
}

// Block user process
void block(pid_t userPid) {
	buf.mtype = (long)userPid;
	sprintf(buf.mtext, "%ld %d %d", (long)getpid(), BLK, 0);
	buf_length = strlen(buf.mtext) + 1;
	if (msgsnd(msgid, &buf, buf_length, 0) < 0) {
		perror("oss: msgsnd");
		exit(1);
	}
}

// Unblock user process
void unblock(pid_t userPid) {
	buf.mtype = (long)userPid;
	sprintf(buf.mtext, "%ld %d %d", (long)getpid(), UBLK, 0);
	buf_length = strlen(buf.mtext) + 1;
	if (msgsnd(msgid, &buf, buf_length, 0) < 0) {
		perror("oss: msgsnd");
		exit(1);
	}
}

// Output child pid table
void pidTable(pid_t P[]) {
	int procNum;
	printf("[");
	for (procNum = 0; procNum < MAXUSERS; procNum++) {
		if (procNum != 0)
			printf(" , ");
		printf("%5ld", (long)P[procNum]);
	}
	printf("]\n");
}
