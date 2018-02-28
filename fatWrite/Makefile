all:	
	gcc -c -fpic myfat.c
	gcc -shared -o libFAT.so myfat.o
	gcc -Wall -fPIC -I. -o msh fat_tester.o -L. -lFAT

clean:
	rm msh
