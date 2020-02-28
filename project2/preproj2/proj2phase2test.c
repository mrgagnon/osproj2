// Maylee Gagnon

#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "processinfo.h"

// These values MUST match the syscall_32.tbl modifications:
#define __NR_cs3013_syscall2 378

/* Write up 
	The PID and values for time tend to change each time it is run. This is expected because the process will 
	change depending on what else is happening. The time is also likely to change. The UID and state do not change. 
	This is also expected because the user ramains the same and the state should be the same will running. The PID for the 
	siblings have been in order.  

*/

long testCall2 (struct processinfo *info) {
        return (long) syscall(__NR_cs3013_syscall2, info);  
} 

void printInfo(struct processinfo info) {
	fprintf(stderr, "State: %ld\n", info.state);
	fprintf(stderr, "PID: %d\n", info.pid);
	fprintf(stderr, "Parent PID: %d\n", info.parent_pid);
	fprintf(stderr, "Youngest child PID: %d\n", info.youngest_child);
	fprintf(stderr, "Younger sibling PID: %d\n", info.younger_sibling);
	fprintf(stderr, "Older sibling PID: %d\n", info.older_sibling);
	fprintf(stderr, "UID of process owner: %d\n", info.uid);
	fprintf(stderr, "Start time (nanoseconds): %lld\n", info.start_time);
	fprintf(stderr, "CPU time user (microseconds): %lld\n", info. user_time);
	fprintf(stderr, "CPU time system (microseconds): %lld\n", info.sys_time);
	fprintf(stderr, "User time children (microseconds): %lld\n", info.cutime);
	fprintf(stderr, "System time children (micoseconds): %lld\n", info.cstime);
}

int main () {
	fprintf(stderr,"\t Null test: %ld\n", testCall2(NULL));	//TODO test if pointer argument is null or invalid  

	struct processinfo info1; 
	fprintf(stderr, "\t Parent only: %ld\n", testCall2(&info1));
	printInfo(info1);
	
	int pid1, pid2; 
	if ((pid1 = fork() ) < 0) {
		fprintf(stderr, "Fork error \n");
		exit(1);	
	} 
	else if (pid1 == 0) { // 1st child 
		sleep(5);
		struct processinfo info2; 
		fprintf(stderr, "\t Child1: %ld\n", testCall2(&info2));
		printInfo(info2);
		exit(0);
	} 
	else { // parent 
		if ((pid2 = fork())<0){ // 2nd child 
			fprintf(stderr, "Fork error \n");
			exit(1);
		}
		else if (pid2 == 0) { 
			sleep(3);
			struct processinfo info3; 
			fprintf(stderr, "\t Child2: %ld\n", testCall2(&info3));
			printInfo(info3);		
		}
		else {
			struct processinfo info4; 
			int waita = wait(0);
			int waitb = wait(0);
			fprintf(stderr, "\t Parent: %ld\n", testCall2(&info4));
			printInfo(info4);
			exit(0);
		} 
	} 
}



