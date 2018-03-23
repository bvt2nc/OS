#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>

int MAX_N = 60000; //2^(16);
long *data;
size_t N;
int NUM_THREADS;
//pthread_barrier_t barrier;

void* getMax(void * arg);
void readData();
int power(int base, int exp);

typedef struct
{
	int tid;
	int active; //logical indicating whether thread should be active
	int delta; 
	int rval;
} thread_arg;

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

int main()
{

	readData();
	printf("N: %d \n", (int)N);	
	NUM_THREADS = N / 2;

	pthread_t tid[NUM_THREADS];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	//pthread_barrier_init(&barrier, NULL, NUM_THREADS + 1);
	barrier = (barrier_s *)malloc(sizeof(barrier_s));
	barrier_init(barrier, NUM_THREADS + 1);
	thread_arg arg[NUM_THREADS];
	int currentN = N;
	int round = 1;

	while(currentN >= 1)
	{
		//printf("Round: %d \n", round);
		//printf("Creating threads...\n");
		int i;
		for(i = 0; i < NUM_THREADS; i++)
		{
			arg[i].tid = i * power(2, round);
			arg[i].delta = power(2, round - 1);
			/*printf("%d %d %d \n", i, power(2, round), i * power(2, round));
			printf("%d: %d \n", i, arg[i].tid);
			printf("currentN: %d\n", currentN);
			printf("==============\n");*/
			if(i < (currentN / 2))	
				arg[i].active = 1;
			else
				arg[i].active = 0;
			pthread_create(&tid[i], &attr, getMax, &arg[i]);
		}
		//printf("==========tids=======\n");
		//pthread_barrier_wait(&barrier);
		barrier_wait(barrier);
		//printf("Joining... \n");
		for(i = 0; i < NUM_THREADS; i++)
		{
			pthread_join(tid[i], NULL);
			if(arg[i].active)
			{
				//fprintf(stdout, "%d %d \n",data[arg[i].tid], data[arg[i].tid + arg[i].delta]);
				//fprintf(stdout, "%d \n", arg[i].rval);
			}
		}
		currentN = currentN / 2;
		round++;
		//fprintf(stdout, "==========================================================\n");
	}
	fprintf(stdout, "%d \n", arg[0].rval);

}

/*
Helper method for pthreads.

Returns the maximum of the elements at position tid and (tid + delta) passed in arg.
Arg is casted as a thread_arg to extract tid.
The max is passed back to arg in rval
*/
void* getMax(void *arg)
{
	//printf("in helper...\n");
	thread_arg *s = (thread_arg*)arg;
	int pos1 = s -> tid;
	int pos2 = pos1 + (s -> delta);
	int active = s -> active;

	if(active)
	{
		//printf("active\n");
		long max = data[pos1];
		//printf("temp max\n");
		long temp;
		/*printf("pos1: %d \n", pos1);
		printf("pos2: %d \n", pos2);
		printf("N: %d \n", (int)N);*/
		if(data[pos2] > data[pos1])
		{
			//printf("in if...\n");
			max = data[pos2];
			temp = data[pos1];
			data[pos1] = data[pos2];
			data[pos2] = temp;
		}
		//printf("putting min back in rval...\n");
		((thread_arg*)arg) -> rval = max;
		//printf("finished...\n");
	}

	//pthread_barrier_wait(&barrier);
	barrier_wait(barrier);
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

	fseek(stdin, 0, SEEK_SET);
	while((fgets(line, sizeof(char) * 509, stdin) != NULL) && (line[0] != '\n'))
	{
		//printf("N: %d \n", (int)N);
		//printf("%s \n", line);
		l = atol(line);
		//printf("l: %ld \n", l);
		data[N] = l;
		N++;
		//printf("data[N]: %ld \n", data[(int)N - 1]);
	}

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
	sem_init(&(barrier -> throttle), 0, 1);	
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