#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

int MAX_N = 60000; //2^(16);
int *data;
size_t N;
int NUM_THREADS;
pthread_barrier_t barrier;

void* getMax(void * arg);
void readData();

typedef struct
{
	int tid;
	int pos1;
	int pos2;
	int rval;
} thread_arg;

int main()
{

	readData();
	NUM_THREADS = N / 2;

	pthread_t tid[NUM_THREADS];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_barrier_init(&barrier, NULL, NUM_THREADS + 1);
	thread_arg arg[NUM_THREADS];

	//printf("Creating threads...\n");
	int i;
	for(i = 0; i < NUM_THREADS; i++)
	{
		arg[i].tid = i * 2;
		pthread_create(&tid[i], &attr, getMax, &arg[i]);
	}
	pthread_barrier_wait(&barrier);
	//printf("Joining... \n");
	for(i = 0; i < NUM_THREADS; i++)
	{
		pthread_join(tid[i], NULL);
		fprintf(stdout, "%d \n", arg[i].rval);
	}

}

/*
Helper method for pthreads.

Returns the maximum of the elements at position tid and (tid + 1) passed in arg.
Arg is casted as a thread_arg to extract tid.
The max is passed back to arg in rval
*/
void* getMax(void *arg)
{
	//printf("in helper...\n");
	thread_arg *s = (thread_arg*)arg;
	int tid = s -> tid;
	//printf("putting min back in rval...\n");
	int max = data[tid + 1];
	if(data[tid] > data[tid + 1])
		max =data[tid];
	((thread_arg*)arg) -> rval = max;
	//printf("finished...\n");

	pthread_barrier_wait(&barrier);
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
	int x, result;
	N = 0;
	data = (int *)malloc(sizeof(int) * MAX_N);

	fseek(stdin, 0, SEEK_SET);
	while((fgets(line, sizeof(line), stdin) != NULL) && (line[0] != '\n'))
	{
		x = atoi(line);
		data[N] = x;
		N++;
	}

	data = (int *) realloc(data, N * sizeof(int));
	//for(x = 0; x < N; x++)
	//	fprintf(stdout, "%d \n", data[x]);
}
