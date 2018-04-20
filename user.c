// user.c
#include "proj5.h"
#define RNDBND 20

int msgid;              // Message queue ID
message_buf buf;        // Message buffer
size_t buf_length;      // Length of send message buffer

void request(int);
void release(int);
void terminate();

int main() {
	int clkid;		// Shared memory ID of simclock

	sim_time *simClock;	// Pointer to simulated system clock

	srand((time(NULL) + getpid()) % UINT_MAX);
	int i;

	int termFlag = 0;

// Setup IPC

	// Locate shared simulated system clock
	if ((clkid = shmget(CLKKEY, sizeof(sim_time), 0666)) < 0 ) {
		perror("user: shmget clkid");
		exit(1);
	}
	simClock = shmat(clkid, NULL, 0);

	// Locate message queue
	if ((msgid = msgget(MSGKEY, 0666)) < 0) {
		perror("user: shmget msgid");
		exit(1);
	}


// Send a few random messages
	request(1);

	msgrcv(msgid, &buf, MSGSZ, (long)getppid(), 0);
	char *ptr;
	pid_t userPid = (pid_t)strtol(buf.mtext, &ptr, 10);
	int msgType = (int)strtol(ptr, &ptr, 10);
	int msgData = (int)strtol(ptr, &ptr, 10);

	//printf("%ld: recieved Resource R%d\n", (long)getpid(), msgData);


	/*
	while (termFlag == 1) {
	
		// Handle messages from oss
		if (msgrcv(msgid, &buf, MSGSZ, (long)getppid(), IPC_NOWAIT) != (ssize_t)-1) {

			// Parse message
			char *ptr;
			pid_t userPid = (pid_t)strtol(buf.mtext, &ptr, 10);
			int msgType = (int)strtol(ptr, &ptr, 10);
			int msgData = (int)strtol(ptr, &ptr, 10);

			// Handle message
			switch (msgType) {
				case ALC:
					printf("ALC message recieved\n");
					termFlag = 1;
					break;
				case BLK:
					printf("BLK message recieved\n");
					break;
				case UBLK:
					printf("UBLK message recieved\n");
					break;
				}
				// Respond to user


		}

	}
	*/
	
	terminate();
	exit(0);
}

// Request resource
void request(int resource) {
	buf.mtype = (long)getppid();
	sprintf(buf.mtext, "%ld %d %d", (long)getpid(), REQ, resource);
	buf_length = strlen(buf.mtext) + 1;
	if (msgsnd(msgid, &buf, buf_length, 0) < 0) {
		perror("user: msgsnd");
		exit(1);
	}
}

// Release resource
void release(int resource) {
	buf.mtype = (long)getppid();
	sprintf(buf.mtext, "%ld %d %d", (long)getpid(), REL, resource);
	buf_length = strlen(buf.mtext) + 1;
	if (msgsnd(msgid, &buf, buf_length, 0) < 0) {
		perror("user: msgsnd");
		exit(1);
	}
}

// Signal oss that user is terminating
void terminate() {
	buf.mtype = (long)getppid();
	sprintf(buf.mtext, "%ld %d 0", (long)getpid(), TERM);
	buf_length = strlen(buf.mtext) + 1;
	if (msgsnd(msgid, &buf, buf_length, 0) < 0) {
		perror("user: msgsnd");
		exit(1);
	}
}
