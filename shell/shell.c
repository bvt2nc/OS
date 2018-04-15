#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>

int getcmd(char *buf, int size);
int parseline(char *buf);
int execcmd(char *cmd);
char * trimwhitespace(char* str);
int mySTDIN;
int mySTDOUT;

const char newLine = '\n';

int main()
{
	//char *const argv[] = {"/bin/cat", "foo.txt", NULL};
	//execv("/bin/cat", argv);
	char *buf = (char *)malloc(sizeof(char) * 101);
	mySTDIN = 0;
	mySTDOUT = 1;
	int status;

	printf(">");
	while(getcmd(buf, sizeof(char) * 101))
	{
		printf(">");
		if(buf[0] == '#')
			continue;
		//If line reads "exit"
		if(buf[0] == 'e' && buf[1] == 'x' && buf[2] == 'i' && buf[3] == 't' && (buf[4] == 0 || isspace(buf[4])))
			break;

		/*char *split = strtok(buf, "><|");
		while(split != NULL)
		{
			printf("%s \n", split);
			split = strtok(NULL, "><|");
		}*/
		if(fork() == 0)
		{
			//printf("%s \n", buf);
			parseline(buf);
			exit(1);
		}
		wait(&status);
	}
}

int getcmd(char *buf, int size)
{
	memset(buf, 0, size);
	fgets(buf, size, stdin);
	if(buf[0] == 0) //No command/EOF
		return 0;
	if(strlen(buf) >= 100)
	{
		printf("Error, line is greater than 100 characters\n");
		return 0;
	}
	return 1;
}

int parseline(char *buf)
{
	int i;
	int index = 0;
	char *cmd = (char *)malloc(sizeof(char) * 100);
	char *cmdList[100];
	char operatorList[100];
	int cmdLen = 0;
	int opLen = 0;
	const char *operators = "><|";
	for(i = 0; i < 101; i++)
	{
		//operator found
		if(buf[i] == '|' || buf[i] == '>' || buf[i] == '<')
		{
			//printf("cmd: ");
			//execcmd(cmd);
			//printf("operator: %c \n", buf[i]);
			cmdList[cmdLen] = trimwhitespace(cmd);
			//printf("cmdList[%d]: %s \n", cmdLen, cmdList[cmdLen]);
			operatorList[opLen] = buf[i];
			cmd = (char *)malloc(sizeof(char) * 100);
			index = 0;
			cmdLen++;
			opLen++;
		}
		//operator not found
		else
		{
			if((index == 0 && buf[i] == ' ') || buf[i] == '\n')
				continue;
			cmd[index] = buf[i];
			index++;
		}
	}
	cmdList[cmdLen] = cmd;
	cmdLen++;
	/*printf("commands: \n");
	for(i = 0; i < cmdLen; i++)
	{
		printf("%s \n", cmdList[i]);
	}
	printf("operators: \n");
	for(i = 0; i < opLen; i++)
	{
		printf("%c \n", operatorList[i]);
	}*/
	//execcmd(cmd);

	if(opLen == 0)
	{
		execcmd(cmdList[0]);
		return 1;
	}

	int cmdIndex = 1;
	int execCMDIndex = 0;
	char op;
	for(i = 0; i < opLen; i++)
	{
		op = operatorList[i];
		if(op == '>')
		{
			int fd = open(cmdList[cmdIndex], O_WRONLY|O_CREAT|O_TRUNC, 0666);
			if(fd < 0)
			{
				printf("There was an error opening %s \n", cmdList[cmdIndex + 1]);
				exit(0);
			}

			close(mySTDOUT);
			//printf("mySTDOUT: %d \n", mySTDOUT);
			//close(1);
			dup(fd);
			mySTDOUT = fd;
			cmdIndex++;
		}
		else if(op == '<')
		{
			int fd = open(cmdList[cmdIndex], O_RDONLY);
			if(fd < 0)
			{
				printf("There was an error opening %s \n", cmdList[cmdIndex + 1]);
				exit(0);
			}

			close(mySTDIN);
			dup(fd);
			mySTDIN = fd;
			//execcmd(cmdList[cmdIndex]);
			cmdIndex++;
		}
		else if(op == '|')
		{
			int p[2], status;
			pipe(p);
			if(fork() == 0)
			{
				//printf("left: pipe executing cmd: %s \n", cmdList[execCMDIndex]);
				close(1);
				dup(p[1]);
				close(p[0]);
				close(p[1]);
				//mySTDOUT = p[1];
				execcmd(cmdList[execCMDIndex]);
			}
			if(fork() == 0)
			{
				//printf("right: \n");
				close(0);
				dup(p[0]);
				close(p[1]);
				close(p[0]);
				//mySTDIN = p[0];
				//printf("cmdIndex: %d \n", cmdIndex);
				//printf("cmdLen: %d \n", cmdLen);
				execCMDIndex = cmdIndex;
				if(cmdIndex >= cmdLen - 1)
				{
					//printf("pipe executing right cmd: %s \n", cmdList[execCMDIndex]);
					execcmd(cmdList[execCMDIndex]);
					exit(1);
				}
				cmdIndex++;
			}
			else
			{
				execCMDIndex = cmdLen;
				break;
			}
			close(p[0]);
			close(p[1]);
			wait(0);
			wait(0);
			//printf("cmdIndex: %d \n", cmdIndex);
			//execCMDIndex = cmdIndex;
			//cmdIndex++;
			//if(cmdIndex >= cmdLen - 1)
			//break;
		}
		else
		{
			printf("Should never reach here \n");
		}
	}
	if(execCMDIndex < cmdLen)
	{
		printf("execCMDIndex: %d \n", execCMDIndex);
		execcmd(cmdList[execCMDIndex]);
	}

	return 1;
}

int execcmd(char* cmd)
{
	char* split = strtok(cmd, " ");
	char* argv[100];
	int N = 0;
	while(split != NULL)
	{
		argv[N] = trimwhitespace(split);
		split = strtok(NULL, " ");
		N++;
	}
	argv[N] = NULL;
	int i;
	/*for(i = 0; i < N; i++)
	{
		printf("argv[%d]: %s \n", i, argv[i]);
	}
	//printf("\n");*/
	if(argv[0][0] != '/')
	{
		//First try running as is
		execv(argv[0], argv);
		
		//Then try appending /bin/
		char temp[100] = {0};
		strcpy(temp, "/bin/");
		strcat(temp, argv[0]);
		execv(temp, argv);

		//Then try appending /usr/bin/
		char temp2[100] = {0};
		strcpy(temp2, "/usr/bin/");
		strcat(temp2, argv[0]);
		execv(temp2, argv);
	}
	else
	{
		/*printf("path: %s \n", argv[0]);
		printf("argv[0]: %s \n", argv[0]);
		printf("argv[1]: %s \n", argv[1]);
		printf("argv[2]: %s \n", argv[2]);*/
		execv(argv[0], argv);
	}

	return 1;
}

char *trimwhitespace(char *str)
{
	while(isspace(*str))
		str++;

	if(*str == 0)
		return str;

	char *end = str + strlen(str) - 1;
	while(end > str && (isspace(*end) || *end == '\n'))
		end--;

	*(end + 1) = 0;
	return str;
}