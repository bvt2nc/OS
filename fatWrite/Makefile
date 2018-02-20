C16:	
	gcc -Wall -fPIC -I. -o msh fat_tester.o -L. -lFAT16

CPP16:
	g++ -Wall -fPIC -I. -o msh fat_tester.o -L. -lFAT16
 
C32:	
	gcc -c -fpic myfat.c
	gcc -shared -o libFAT32.so myfat.o
	gcc -Wall -fPIC -I. -o msh fat_tester.o -L. -lFAT32

CPP32:
	g++ -Wall -fPIC -I. -o msh fat_tester.o -L. -lFAT32

CBOTH:	
	gcc -Wall -fPIC -I. -o msh fat_tester.o -L. -lFAT

CPPBOTH:
	g++ -Wall -fPIC -I. -o msh fat_tester.o -L. -lFAT

clean:
	rm msh
