#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <vector>
#include <chrono>
#include <sys/time.h>
#include <semaphore.h>
#include <fcntl.h> // O_CREAT
#include <sched.h>
#include <time.h> // nanosleep
#include <sys/mman.h>
#include <sys/resource.h> // PRIO_PROCESS

#define MAX_THREADS 36

//#define DEBUG
//#define ELAPSED_TIME // Uncomment to measure elapsed time (microseconds)

int main(int argc, char** argv) {
	volatile char* lock; // Locking mechanism based on shared memory and atomic operations

	// Arguments passed to the child program (lib.so is hooks the _exit
	// function and dump the memory mapping)
	char* arguments[] = {argv[1], NULL};
	char preload[] = "LD_PRELOAD=./lib.so";
	char* environment[] = {preload, NULL};

	long int affinities[MAX_THREADS]; // affinities[0] contains the CPU to run the first instance on
	size_t num_thread = 0; // How many thread we'll run

	if (argc < 3) {
		puts("Usage: ./<binary's name> <program to execute> <cpu> [<cpu 2nd thread> <cpu 3rd thread> ...]");
		return 1;
	}

	if (argc-2 > MAX_THREADS) {
		puts("Error: too many threads, change MAX_THREADS and recompile");
		return 4;
	}

	// Parse input
	for (int i = 2; i < argc; i++) {
		affinities[i-2] = atoi(argv[i]); // default: 0
		num_thread ++;
	}

	// Lock the child processes while we setup all childs
	lock = (char*)mmap(NULL, 1, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*lock = 0;


	// Start and setup all childs
	size_t running_threads = 0;
	while (running_threads < num_thread) {
		pid_t fork_pid = fork();
		if (fork_pid == 0) {
			while (*lock != 42); // Wait for other processes
			execvpe(argv[1], arguments, environment);
			puts("Error command not found");
			return 2;
		} else if (fork_pid < 0){
			puts("Error forking went wrong");
			return 3;
		}
		// else: fork_pid > 0: We are in the main thread

		// Setting Policy SCHED_RR to priority maximum
		// Note with SCHED_FIFO the first thread is faster
		struct sched_param p;
		p.sched_priority = 99;
		if (sched_setscheduler(fork_pid, SCHED_RR, &p) != 0)
			puts("Error Cannot set scheduler priority");
		
#ifdef DEBUG
		int min = sched_get_priority_min(SCHED_FIFO);
		int max = sched_get_priority_max(SCHED_FIFO);
		printf("Min/Max prio: %d/%d", min, max);
#endif

		// Setting priority (Nice)
		setpriority(PRIO_PROCESS, fork_pid, -20);

		// Set the affinity
		cpu_set_t aff_mask;
		CPU_ZERO(&aff_mask);
		CPU_SET(affinities[running_threads], &aff_mask); // CPU 0
		if(sched_setaffinity(fork_pid, sizeof(cpu_set_t), &aff_mask) == -1)
			puts("Error Cannot set scheduler CPU");

		printf("PID %d - CPU %ld\n", fork_pid, affinities[running_threads]);
		running_threads++;
	}

	// Delay for the last thread to spawn in the right CPU
	struct timespec ts = {0, 100};
	nanosleep(&ts, &ts);

#ifdef ELAPSED_TIME
	// Measure the start time
	struct timeval t_start;
	gettimeofday(&t_start, NULL);
#endif

	// Unlock the lock for all threads at the same time
	__sync_add_and_fetch(lock, 42);
	msync((void*)lock, 1, MS_SYNC | MS_INVALIDATE);

	// Wait for every child to return
	int wait_status;
	while (running_threads) {
#ifdef ELAPSED_TIME
		pid_t returned = wait(&wait_status);
		struct timeval t_end;
		gettimeofday(&t_end, NULL);
		int elapsed = ((t_end.tv_sec - t_start.tv_sec) * 1000000) + (t_end.tv_usec - t_start.tv_usec);
		printf("%d - %d\n", returned, elapsed);
#else
		wait(&wait_status);
#endif
		running_threads--;
	}

	munmap((void*)lock, 1);
	return 0;
}
