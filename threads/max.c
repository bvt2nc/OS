/*
Binary Reduction using pthreads with self-made barrier using semaphores

CS4414 Operating Systems
Spring 2018

Benjamin Trans (bvt2nc)

max.c  - Binary reduction that takens in a list of numbers from stdin and outputs the max 
		 to stdout
stdin  - Each number should be in its own line. File ends with a new line.
stdout - The maximum value read in will be outputted and only the maximum value

Code written solely by Benjamin Trans. Acknowledgements to Professor Andrew Grimshaw 
for barrier with semaphore implementation that was directly used from the example
shown in class.

The following code implements a binary reduction using N / 2 threads reused throughout
each round where N is an even number of numbers (casted as floats) read in from stdin.
Synchronization was accomplished with a self-made barrier using the POSIX counting semaphores
acting as a binary semaphore.

We refer the reader to the assignment writeup for all of the details.

	COMPILE:		make
	MAKEFILE:		Makefile

	MODIFICATIONS:
			March 20 -  Start assignment by making Makefile and basic helloworld
			March 21 -  Developed input and output functions to read stdin and output to 
						stdout. Developed Basic 1 round implementation of maximum binary
						reduction using pthreads.
			March 21 -  Full implemenation using built-in barrier. Threads are not resused
			March 22 - 	Implement barrier with POSIX semaphores. Create simple program to 
						generate input file
			March 23 - 	Fix reading in the numbers, changing reading in ints to floats.
						Fix the max number limit read in to be sizeof(char) * 509
			March 25 - 	Reuse threads and document code.
*/

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>

int MAX_N = 60000;
long *data;
size_t N;
int NUM_THREADS;

void* getMax(void * arg);
void readData();
int power(int base, int exp);

typedef struct
{
	int tid;
	//int active; //logical indicating whether thread should be active
	//int delta; 
	int rval;
} thread_arg;

/*
Taken from Grimshaw's lecture
*/
typedef struct 
{
	int value;
	int init;
	sem_t mutex;
	sem_t waitq;
	sem_t throttle;
} barrier_s;

void barrier_init(barrier_s *barrier, int init);
void barrier_wait(barrier_s *barrier);

barrier_s *barrier;
barrier_s *finishBarrier;

int main()
{

	readData();
	NUM_THREADS = N / 2;

	//initialize threads and barriers
	pthread_t tid[NUM_THREADS];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	//barrier to synchronize each round that each thread processes
	barrier = (barrier_s *)malloc(sizeof(barrier_s));
	//barrier to synchronize when each thread is finished ALL rounds
	finishBarrier = (barrier_s *)malloc(sizeof(barrier_s));
	barrier_init(barrier, NUM_THREADS);
	barrier_init(finishBarrier, NUM_THREADS + 1);
	thread_arg arg[NUM_THREADS];

	//printf("Creating threads...\n");
	int i;
	for(i = 0; i < NUM_THREADS; i++)
	{
		//Set parameter
		arg[i].tid = i;
		//Create threads
		pthread_create(&tid[i], &attr, getMax, &arg[i]);
	}

	//Wait for all threads to finish ALL rounds of binary reduction 
	barrier_wait(finishBarrier);
	//printf("Joining... \n");
	for(i = 0; i < NUM_THREADS; i++)
	{
		//Join all threads
		pthread_join(tid[i], NULL);
	}
	//Print the max found to stdout
	fprintf(stdout, "%d \n", arg[0].rval);

}

/*
Helper method for pthreads.

Returns the maximum of the elements at position pos1 and pos2 that is calculated
based on the tid and round.
Arg is casted as a thread_arg to extract tid.
The max is passed back to arg in rval
*/
void* getMax(void *arg)
{
	//printf("in helper...\n");
	//Extract arguments and initialize for the rounds
	thread_arg *s = (thread_arg*)arg;
	int currentN = N;
	int round = 1;
	int tid = s -> tid;

	//Repeat until we are only left with one number to compare (the max)
	while(currentN >= 1)
	{
		//printf("round: %d \n", round);

		//Calculate pos1 and pos2
		int pos1 = tid * power(2, round);
		int pos2 = pos1 + power(2, round - 1);

		//If thread not out of scope, find the max
		if(pos2 < N)
		{
			//printf("active\n");
			long max = data[pos1];
			//printf("temp max\n");
			long temp;
			/*printf("pos1: %d \n", pos1);
			printf("pos2: %d \n", pos2);
			printf("N: %d \n", (int)N);*/

			//Set new max and swap max to always be in pos1
			if(data[pos2] > data[pos1])
			{
				//printf("in if...\n");
				max = data[pos2];
				temp = data[pos1];
				data[pos1] = data[pos2];
				data[pos2] = temp;
			}

			//Set the thread's rval
			//printf("putting min back in rval...\n");
			((thread_arg*)arg) -> rval = max;
			//printf("finished...\n");
		}

		//Wait until all other threads and finished with their round
		//printf("before tid: %d, round: %d, value: %d \n", tid, round, barrier -> value);
		barrier_wait(barrier);
		//printf("after tid: %d, round: %d, value: %d \n", tid, round, barrier -> value);

		//Increment round and cut the number of data elements we are pairing in half
		currentN = currentN / 2;
		round++;

	}

	//Wait until ALL threads completely finished
	//pthread_barrier_wait(&barrier);
	barrier_wait(finishBarrier);
	return NULL;
}

/*
Reads stdin line by line until a new line is reached. Puts each integer found
into data.

Data is initialized to an array of size MAX_N, and then resized with realloc
at the end.
*/
void readData()
{
	char *line = (char *)malloc(sizeof(char) * 509); //gcc gives warning after 509
	long l, result;
	N = 0;
	data = (long *)malloc(sizeof(long) * MAX_N);

	//Read each line
	fseek(stdin, 0, SEEK_SET);
	while((fgets(line, sizeof(char) * 509, stdin) != NULL) && (line[0] != '\n'))
	{
		//printf("N: %d \n", (int)N);
		//printf("%s \n", line);
		l = atol(line); //Cast string to long
		//printf("l: %ld \n", l);
		data[N] = l;
		N++;
		//printf("data[N]: %ld \n", data[(int)N - 1]);
	}

	//Resize data array to exact size of N elements
	data = (long *) realloc(data, N * sizeof(long));
	/*int x;
	for(x = 0; x < N; x++)
		fprintf(stdout, "%ld \n", data[x]);*/
}

/*
Function that handles exponentiation
*/
int power(int base, int exp)
{
	int i;
	int rval = 1;
	for(i = 0; i < exp; i++)
		rval *= base;
	return rval;
}

/*
Initialize barrier
Sets mutex value to 1; Sets waitq value to 0; Sets throttle value to 1;
*/
void barrier_init(barrier_s *barrier, int init)
{
	barrier -> value = init;
	barrier -> init = init;
	sem_init(&(barrier -> mutex), 0, 1);
	sem_init(&(barrier -> waitq), 0, 0);
	sem_init(&(barrier -> throttle), 0, 0);	
}

/*
Implement equivalent of pthread_barrier_wait()
Taken from the implementation showed in class my Prof. Grimshaw
*/
void barrier_wait(barrier_s *barrier)
{
	int i;
	int init = barrier -> init;

	sem_wait(&(barrier -> mutex));
	barrier -> value--;
	if(barrier -> value != 0)
	{
		sem_post(&(barrier -> mutex));
		sem_wait(&(barrier -> waitq));
		sem_post(&(barrier -> throttle));
	}
	else
	{
		for(i = 0; i < (init - 1); i++)
		{
			sem_post(&(barrier -> waitq));
			sem_wait(&(barrier -> throttle));
		}
		barrier -> value = barrier -> init;
		sem_post(&(barrier -> mutex));
	}
}
