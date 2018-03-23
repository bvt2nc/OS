#include <stdio.h>
#include <stdlib.h>

int main()
{
	int N = 8192;
	int i;
	int max = 999999993	;
	int data[N];

	for(i = 0; i < N; i++)
		data[i] = rand() % (max + 1);

	//insert ma somewhxere in data
	int pos = rand() % N;
	data[pos] = max;

	for(i = 0; i < N; i++)
		printf("%d\n", data[i]);
}