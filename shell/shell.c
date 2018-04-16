/*
Shell Implementation

CS4414 Operating Systems
Spring 2018

Benjamin Trans (bvt2nc)

shell.c 	- Code that parses and executes each line inputted in stdin
stdin  		- Valid unix command to run. "exit" or EOF will exit out of the shell
stdout 		- After each line is read in, the line will be executed and a status code 
			- will be printed out for each command executed in the line. If a command 
			- failed to execute

Code written solely by Benjamin Trans.

The following code implements a self made shell as specifed by the assignment sheet.
The shell prompt is ">". THe shell does not support environment variables. This shell
supports basic commands, is able to run programs, and supports file redirection and pipes.

We refer the reader to the assignment writeup for all of the details.

	COMPILE:		make
	MAKEFILE:		Makefile

	MODIFICATIONS:
			April 14 -  Began assignment. Implemented reading in from stdin in and allowing
						only 100 characters per line.
			April 15 - 	Support for single commands. Moved on to file redirection and
						started pipes.

						Finished pipes, output still messy
			April 16 - 	Document code. Fixed pops and rebased output to match
						assignment description. 
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>

int getcmd(char *buf, int size);
int executetoken(char *buf);
int parseline(char *buf);
int execcmd(char *cmd);
char * trimwhitespace(char* str);

const char newLine = '\n';

int main()
{
	//char *const argv[] = {"/bin/cat", "foo.txt", NULL};
	//execv("/bin/cat", argv);

	//Buffer to read in each line... size is 101 rather than 100 to account for \n to enter
	char *buf = (char *)malloc(sizeof(char) * 101);
	int status, retCode, i;
	pid_t pid, child_pid;

	//Print initial shell prompt
	printf(">");
	fflush(stdout);
	//Continually read from stdin into buffer
	while(getcmd(buf, sizeof(char) * 101))
	{
		//For testing purposes... should remember to comment out before submission
		//if(buf[0] == '#')
		//	continue;

		//If line reads "exit" stop
		if(buf[0] == 'e' && buf[1] == 'x' && buf[2] == 'i' && buf[3] == 't' && (buf[4] == 0 || isspace(buf[4])))
			break;

		child_pid = fork();
		if(child_pid == 0) //So that the main process doesn't exit
		{
			retCode = parseline(buf); //Parse the line and execute all commands
			//Below should not be reached... only used for debugging purposes
			if(retCode == 0) //If any commands failed
			{
				exit(-1);
			}
			exit(0);
		}
		while((pid = wait(&status)) > 0); //Wait until all child processes are done

		//Reprint shell prompt
		printf(">");
		fflush(stdout);
	}
}

//Gets line from stdin and copies it to buffer
int getcmd(char *buf, int size)
{
	//Reinitialize buffer
	memset(buf, 0, size);
	//Read line
	fgets(buf, size, stdin);
	if(buf[0] == 0) //No command/EOF
		return 0;
	//Check if #characters are within specified bounds
	if(strlen(buf) >= 100)
	{
		printf("Error, line is greater than 100 characters\n");
		return 0;
	}
	return 1;
}

/*
Parse the line, execute all commands in line and do all redirection

buf - The line to be parsed

Returns 1 on success, 0 on failure
*/
int parseline(char *buf)
{
	int i;
	int index = 0;
	char *cmd = (char *)malloc(sizeof(char) * 100);
	char *cmdList[100];
	char operatorList[100];
	int cmdLen = 0;
	int opLen = 0;
	int status, laststatus;
	pid_t child1, child2, pid;
	const char *operators = "><|";

	//Loop through each character in buffer
	for(i = 0; i < 101; i++)
	{
		//If end of line is reached
		if(buf[i] == 0)
			break;

		//operator found
		if(buf[i] == '|' || buf[i] == '>' || buf[i] == '<')
		{
			//printf("cmd: ");
			//execcmd(cmd);
			//printf("operator: %c \n", buf[i]);

			//Stash the command
			cmdList[cmdLen] = trimwhitespace(cmd);
			//Stash the operator
			operatorList[opLen] = buf[i];
			//Realloc our cmd pointer to be reused since we have already stashed it
			cmd = (char *)malloc(sizeof(char) * 100);
			index = 0;
			cmdLen++;
			opLen++;
		}
		//operator not found
		else
		{
			//Don't want any leading whitespaces or and new lines
			if((index == 0 && buf[i] == ' ') || buf[i] == '\n')
				continue;
			cmd[index] = buf[i];
			index++;
		}
	}
	//Stash what is left over in command unless the command is empty
	if(cmd[0] != 0)
	{
		cmdList[cmdLen] = cmd;
		cmdLen++;
	}

	//There has to be more commands than operators... otherwise fail
	if(opLen >= cmdLen)
	{
		printf("Incorrect input\n");
		return 0;
	}

	//If there are no operators just run the command
	if(opLen == 0)
	{
		if(fork() == 0)
			execcmd(cmdList[0]);
		else
		{
			wait(&status);
			if(status != 0)
			{
				printf("Command %s failed to execute\n", cmdList[0]);
				return 0;
				exit(1);
			}
			printf("%s exited with exit code 0\n", cmdList[0]);
			exit(0);
		}
		return 0;
	}

	//First command in list will always be executed first
	//From there on, keep a pointer of the next command found and decide based on operator if
	//it is a file redirection or command
	int cmdIndex = 1;
	int execCMDIndex = 0;
	char op;
	//Save stdin and stdout to be reopened
	int mySTDOUT;
	int mySTDIN;
	int stdoutChanged = 0;
	//Loop through all operators found
	for(i = 0; i < opLen; i++)
	{
		op = operatorList[i];
		//Redirect stdout
		if(op == '>')
		{
			//Open the file at the current command pointer to write to 
			int fd = open(cmdList[cmdIndex], O_WRONLY|O_CREAT|O_TRUNC, 0666);

			if(fd < 0)
			{
				printf("There was an error opening %s \n", cmdList[cmdIndex + 1]);
				exit(0);
			}

			//printf("\n"up

			//Redirect stdout and save it to be reopened
			mySTDOUT = dup(1);
			dup2(fd, 1);
			cmdIndex++;
			stdoutChanged = 1;
		}
		//Redirect stdin
		else if(op == '<')
		{
			//Open the file at the current command pointer to read from
			int fd = open(cmdList[cmdIndex], O_RDONLY);

			if(fd < 0)
			{
				printf("There was an error opening %s \n", cmdList[cmdIndex + 1]);
				exit(0);
			}

			//Redirect stdin and save it to be reopened
			mySTDIN = dup(0);
			dup2(fd, 0);
			//execcmd(cmdList[cmdIndex]);
			cmdIndex++;
		}
		else if(op == '|')
		{
			//Initialize pipe and pid for forks
			int p[2];
			pipe(p);

			//Fork the command  to execute in child process
			if((child1 = fork()) == 0)
			{
				close(1);
				dup(p[1]);
				close(p[0]);
				close(p[1]);
				return execcmd(cmdList[execCMDIndex]);
			}
			//parent process
			else
			{
				//wait for child and look at exit code
				wait(&status);
				//printf("pipe status: %d \n", status);
				if(status != 0)
				{
					printf("Command %s failed to execute\n", cmdList[execCMDIndex]);
					return 0;
					exit(1);
				}
				printf("%s exited with exit code 0\n", cmdList[execCMDIndex]);

				close(0);
				dup(p[0]);
				close(p[1]);
				close(p[0]);

				execCMDIndex = cmdIndex;

				//If we have reached the last command in the list, execute it
				if(cmdIndex >= cmdLen - 1)
				{
					//fork the command so that we can output success or failure
					child2 = fork();
					if(child2 == 0)
					{
						execcmd(cmdList[execCMDIndex]);
						//printf("Command %s failed to execute\n", cmdList[execCMDIndex]);
						return 0;
						exit(1);
					}
					else
					{
						//output exit code
						waitpid(child2, &status, WCONTINUED);
						//printf("status: %d \n", status);
						if(status != 0)
						{
							printf("Command %s failed to execute\n", cmdList[execCMDIndex]);
							return 0;
							exit(1);
						}
						printf("%s exited with exit code 0\n", cmdList[execCMDIndex]);
						exit(0);
					}
				}
				cmdIndex++;
			}
		}
		else
		{
			printf("Should never reach here \n");
		}
	}
	//Execute the last command in the list... only reached by redirection
	//This is to ensure that all redirections are made before executing command
	//Commands as a result of pipes do not get here
	if(execCMDIndex < cmdLen)
	{
		if((child1 = fork()) == 0)
			execcmd(cmdList[execCMDIndex]);
		else
		{
			while(waitpid(child1, &laststatus, WCONTINUED) > 0);
			//reconnect stdout
			if(stdoutChanged)
			{
				dup2(mySTDOUT, 1);
				close(mySTDOUT);
				stdoutChanged = 0;
			}
			//dup2(mySTDIN, 0);
			//close(mySTDIN);
			if(laststatus != 0)
			{
				fprintf(stdout, "Command %s failed to execute\n", cmdList[execCMDIndex]);
				return 0;
			}
			fprintf(stdout, "%s exited with exit code 0\n", cmdList[execCMDIndex]);
		}
	}

	return 1;
}

//Execute the command
//Splits cmd into arguments and extract the command
int execcmd(char * cmd)
{
	//Split arguments by whitespace
	char* split = strtok(cmd, " ");
	char* argv[100];
	int N = 0;
	pid_t pid;
	int status;
	//Fill in argv while trimming whitespace
	while(split != NULL)
	{
		argv[N] = trimwhitespace(split);
		split = strtok(NULL, " ");
		N++;
	}
	//Create null terminator
	argv[N] = NULL;
	int i;
	//If the command to execute in argv[0] isn't absolute
	//printf("executing: %s \n", argv[0]);
	if(argv[0][0] != '/')
	{
		//First try running as is
		execv(argv[0], argv);
		
		//Then try prepending /bin/
		char temp[100] = {0};
		strcpy(temp, "/bin/");
		strcat(temp, argv[0]);
		execv(temp, argv);

		//Then try prepending /usr/bin/
		char temp2[100] = {0};
		strcpy(temp2, "/usr/bin/");
		strcat(temp2, argv[0]);
		execv(temp2, argv);

		//fprintf(stdout, "Command %s failed to execute\n", argv[0]);
		return 0;
		exit(1);
	}
	//Absolute
	else
	{
		execv(argv[0], argv);

		//fprintf(stdout, "Command %s failed to execute\n", argv[0]);
		return 0;
		exit(1);
	}

	return 1;
}

/*
Removes leading and trailing whitespace
*/
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

//===============================================================MISC========================================================
		/*//Get tokens
		char *tokens[100];
		char *cmd = (char *)malloc(sizeof(char) * 100);
		int tokensLen = 0;
		int index = 0;
		for(i = 0; i < 100; i++)
		{
			//If end of line is reached
			if(buf[i] == 0)
				break;

			//If pipe found
			if(buf[i] == '|' && i > 0 && i < 99)
			{
				if(buf[i - 1] == ' ' && buf[i + 1] == ' ')
				{
					//Stash the token
					tokens[tokensLen] = trimwhitespace(cmd);
					//Realloc our cmd pointer to be reused since we have already stashed it
					cmd = (char *)malloc(sizeof(char) * 100);
					index = 0;
					tokensLen++;
				}
			}
			//pipe not found
			else
			{
				//Don't want any leading whitespaces or and new lines
				if((index == 0 && buf[i] == ' ') || buf[i] == '\n')
					continue;
				cmd[index] = buf[i];
				index++;
			}
		}
		//Stash what is left over in command unless the command is empty
		if(cmd[0] != 0)
		{
			tokens[tokensLen] = cmd;
			tokensLen++;
		}

		if(tokensLen == 1)
		{
			if(fork() == 0)
				executetoken(tokens[0]);
			wait(NULL);
			printf(">");
			fflush(stdout);
			continue;
		}

		pid_t left, right;
		int leftStatus, rightStatus;
		for(i = 0; i < tokensLen; i++)
		{
			int p[2];
			pipe(p);
			
			left = fork();
			if(left == 0)
			{
				printf("left\n");
				close(1);
				dup(p[1]);
				//dup2(p[1], 1);
				close(p[1]);
				close(p[0]);
				executetoken(tokens[i]);
				printf("Something went wrong\n");
			}
			//right = fork();
			//if(right == 0)
			else
			{
				waitpid(left, &leftStatus, WCONTINUED);
				printf("finished waiting\n");
				close(0);
				dup(p[0]);
				close(p[1]);
				close(p[0]);
				
				if(i >= tokensLen - 1)
				{
					printf("in right execute\n");
					executetoken(tokens[i]);
					printf("something went wrong");
				}
			}
			//waitpid(right, &rightStatus, WCONTINUED);
			//Parent... don't continue or else we will get duplicates
			//else
			//{
			//}
		}*/
		/*for(i = 0; i < tokensLen; i++)
		{
			//waitpid()
		}*/

/*
Parse the line, execute all commands in line and do all redirection

buf - The line to be parsed

Returns 1 on success, 0 on failure
*/
int executetoken(char *buf)
{
	int i;
	int index = 0;
	char *cmd = (char *)malloc(sizeof(char) * 100);
	char *cmdList[100];
	char operatorList[100];
	int cmdLen = 0;
	int opLen = 0;
	const char *operators = "><";

	//Loop through each character in buffer
	for(i = 0; i < 101; i++)
	{
		//If end of line is reached
		if(buf[i] == 0)
			break;

		//operator found
		if(buf[i] == '>' || buf[i] == '<')
		{
			//Stash the command
			cmdList[cmdLen] = trimwhitespace(cmd);
			//Stash the operator
			operatorList[opLen] = buf[i];
			//Realloc our cmd pointer to be reused since we have already stashed it
			cmd = (char *)malloc(sizeof(char) * 100);
			index = 0;
			cmdLen++;
			opLen++;
		}
		//operator not found
		else
		{
			//Don't want any leading whitespaces or and new lines
			if((index == 0 && buf[i] == ' ') || buf[i] == '\n')
				continue;
			cmd[index] = buf[i];
			index++;
		}
	}
	//Stash what is left over in command unless the command is empty
	if(cmd[0] != 0)
	{
		cmdList[cmdLen] = cmd;
		cmdLen++;
	}

	//There has to be more commands than operators... otherwise fail
	if(opLen >= cmdLen)
		return 0;

	//If there are no operators just run the command
	if(opLen == 0)
	{
		return execcmd(cmdList[0]);
	}

	//First command in list will always be executed first
	//From there on, keep a pointer of the next command found and decide based on operator if
	//it is a file redirection or command
	int cmdIndex = 1;
	int execCMDIndex = 0;
	char op;
	//Loop through all operators found
	for(i = 0; i < opLen; i++)
	{
		op = operatorList[i];
		//Redirect stdout
		if(op == '>')
		{
			//Open the file at the current command pointer to write to 
			int fd = open(cmdList[cmdIndex], O_WRONLY|O_CREAT|O_TRUNC, 0666);

			if(fd < 0)
			{
				printf("There was an error opening %s \n", cmdList[cmdIndex + 1]);
				exit(0);
			}

			//printf("\n");

			//Close stdout
			close(1);
			//printf("mySTDOUT: %d \n", mySTDOUT);
			//close(1);
			//Redirect stdout to fd
			dup(fd);
			cmdIndex++;
		}
		//Redirect stdin
		else if(op == '<')
		{
			//Open the file at the current command pointer to read from
			int fd = open(cmdList[cmdIndex], O_RDONLY);

			if(fd < 0)
			{
				printf("There was an error opening %s \n", cmdList[cmdIndex + 1]);
				exit(0);
			}

			//Close stdin
			close(0);
			//Redirect stdin to fd
			dup(fd);
			//execcmd(cmdList[cmdIndex]);
			cmdIndex++;
		}
		else
		{
			printf("Should never reach here \n");
		}
	}
	//Execute the last command in the list... only reached by redirection
	//This is to ensure that all redirections are made before executing command
	//Commands as a result of pipes do not get here
	if(execCMDIndex < cmdLen)
	{
		return execcmd(cmdList[execCMDIndex]);
	}

	return 1;
}