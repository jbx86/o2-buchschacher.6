#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <limits.h>
#include <sys/wait.h>

#define SEMKEY 2424
#define DESCKEY 1077
#define CLKKEY 1138
#define MSGKEY 4242
#define MSGSZ 256
#define NPS 1000000000
#define MAXUSERS 5
#define SIZE 7

#define REQ 1
#define REL 2
#define TERM 3
#define ALC 4
#define BLK 5
#define UBLK 6

typedef struct {
	unsigned int sec;
	unsigned int nano;
} sim_time;

typedef struct msgbuf {
	long mtype;
	char mtext[MSGSZ];
} message_buf;

typedef struct {
	int C[MAXUSERS];
	int A[MAXUSERS];
	int CA[MAXUSERS];
	int R;
	int V;
} descriptor;

// Add a number of nanoseconds to a sim_time
void simadd(sim_time *time, unsigned int incSec, unsigned int incNano) {
	if ((incSec < 0) || (incNano < 0)) {
		fprintf(stderr, "simadd error: function cannot be called with a negative increment\n");
		exit(1);
	}

	time->sec += incSec;	
	time->nano += incNano;

	while (time->nano >= NPS) {
		time->sec += (time->nano / NPS);
		time->nano = (time->sec % NPS);
	}
}

// Calculate number of time that has passed between two times
int simdiff(sim_time simclock, sim_time epoch) {
	if (simclock.sec < epoch.sec) {
		fprintf(stderr, "simdiff error: first time must be greater than second time");
		exit(1);
	}
	else if((simclock.sec == epoch.sec) && (simclock.nano < epoch.nano)) {
		fprintf(stderr, "simdiff error: first time must be greater than second time");
		exit(1);
	}

	while (simclock.sec > epoch.sec) {
		simclock.sec--;
		simclock.nano += NPS;
	}
	
	return simclock.nano - epoch.nano;
}

// Input random number and percentage, return 1 percentage% of the time 
int pctToBit(int roll, int percent) {
	if ((roll % 100) < percent)
		return 1;
	return 0;
}
