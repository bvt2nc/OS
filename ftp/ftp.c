/*
FTP Minimum Implementation with LIST

CS4414 Operating Systems
Spring 2018

Benjamin Trans (bvt2nc)

ftp.c 	- FTP server implementation with 1 command line argument specifying the port number.

Code written solely by Benjamin Trans. Acknowledgements to Professor Andrew Grimshaw 
and wikipedia Berekely Sockets article for bare bones FTP server code.

The following code implements a minimum FTP server with LIST command.

We refer the reader to the assignment writeup for all of the details.

	COMPILE:		make
	MAKEFILE:		Makefile

	MODIFICATIONS:
			April 28 -  Start assignment. Implement Berekely Socket FTP server. Implement successful
						connection with client.
			April 29 - 	Implemented LIST and PORT commands
			April 30 -	Implemented STOR and RETR commands with other basic command needed with minimum
						implementation
			May 1	 - 	Code and algorithm clean up. Document code. Prepare for submission.
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

void sendMessage(int socket, void * message, int len);

int binary = 0; //If we are in binary mode

/*
Essentially all code here...
Barebones FTP taken from Berkeley socket wikipedia page
Listens for commands. Parse and execute commands sent
*/
int main(int argc, char* argv[])
{
	struct sockaddr_in sa;
	//data structures to recieve client message and then parse it by line
	char clientMsg[1024], clientParse[1024];
	//data structures to store each command and pathname of client message
	char command[5], pathname[50];
	//data structure to house message sent to client
	char message[1024];
	ssize_t recsize, sendsize;
	int i;
	int quitLogical = 0;
	//Server socket
	int SocketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//Socket used to send data back and forth between server and client
	int transferSocket;
	if (SocketFD == -1) 
	{
		perror("cannot create socket");
		exit(EXIT_FAILURE);
	}

	memset(&sa, 0, sizeof sa);

	sa.sin_family = AF_INET;
	sa.sin_port = htons((u_short)atoi(argv[1]));
	sa.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (bind(SocketFD, (struct sockaddr *)&sa, sizeof(sa)) == -1) 
	{
		perror("bind failed");
		close(SocketFD);
		exit(EXIT_FAILURE);
	}

	if (listen(SocketFD, 10) == -1) 
	{
		perror("listen failed");
		close(SocketFD);
		exit(EXIT_FAILURE);
	}

	//Wait forever for a client to connect unless shutdown signal sent
	for (;;) 
	{	
		//Client socket that is accepted
		int ConnectFD = accept(SocketFD, NULL, NULL);

		if (0 > ConnectFD) 
		{
			perror("accept failed");
			close(SocketFD);
			exit(EXIT_FAILURE);
		}

		//Send client that they have been connected
		memset(&message[0], 0, sizeof(message));
		strcpy(&message[0], "220 Service ready for new user.\r\n");
		sendsize = send(ConnectFD, (void*)&message[0], sizeof(message), 0);
		if(sendsize < 0)
		{
			printf("Could not send 220\n");
			fprintf(stderr, "%s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		memset(&message[0], 0, sizeof(message));

		//Listen to commands sent by cient until they quit
		for(;;)
		{
			//Get message from client
			recsize = recv(ConnectFD, &clientMsg[0], 1024, 0);
			if (recsize < 0) 
			{
				fprintf(stderr, "%s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
			memset(&clientParse[0], 0, sizeof(clientParse));
			
			//Put only recsize bytes from clientMsg into clientParse
			for(i = 0; i < recsize; i++)
				clientParse[i] = clientMsg[i];
			clientParse[recsize] = '\0';

			/*printf("recsize: %d\n", (int)recsize);
			printf("===================clientParse=================\n");
			for(i = 0; i < 1024; i++)
				printf("%c", clientParse[i]);
			printf("\n==============================================\n");*/

			//Parse clientParse by line... accounts for when more than 1 command are sent at a time
			//as encountered
			char *buffer = strtok(clientParse, "\n\r");
			while(buffer != NULL)
			{
				//Clear out command, pathname, message and scan buffer for command and pathname
				memset(&command[0], 0, sizeof(command));
				memset(&pathname[0], 0, sizeof(pathname));
				memset(&message[0], 0, sizeof(message));
				sscanf(buffer, "%s %s", command, pathname);
				
				//Because the example from Berkeley socket wikipedia did this
				sleep(1);

				//Because our root is the server's cwd
				if(pathname[0] == '/')
					memmove(&pathname[0], &pathname[1], sizeof(pathname) - 1);

				/*printf("======================DATAGRAM=================\n");
				printf("%s\n", buffer);
				printf("===============================================\n");
				//printf("buffer: %s\n", buffer);
				printf("command: %s\n", command);
				printf("pathname: %s\n", pathname);*/

				/*for(i = 0; i < 5; i++)
				{
					command[i] = tolower(command[i]);
				}*/
				
				//If else statements to check for supported commands
				if(!strcmp(command, "LIST"))
				{
					//Let client know that we will begin transferring
					strcpy(message, "125 Transferring...\n");
					//send(ConnectFD, message, 1024, 0);
					sendMessage(ConnectFD, message, 1024);
					memset(&message[0], 0, sizeof(message));

					//Create system command based on command parameter "pathname"
					//Instead of using pipes to transfer data, bypass pipe buffer limit of 65k by putting output
					//into a file named to hopefully avoid collision
					char cmd[100];
					if(pathname[0] == 0)
						strcpy(cmd, "ls -l > LIST.abcxyz 2>LISTerror.abcxyz");
					else
					{
						strcpy(cmd, "ls -l ");
						strcat(cmd, pathname);
						strcat(cmd, "> temp.txt 2>error");
					}
					//printf("cmd: %s\n", cmd);
					FILE *f;
					//Execute cmd
					int status = system(cmd);
					if(status == -1)
						printf("system call failed\n");

					//open output file and send contents of file byte by byte
					f = fopen("LIST.abcxyz", "r");
					i = 0;
					char c;
					while(!feof(f)){
						c = fgetc(f);
						if(c == -1)
							break;
						send(transferSocket, &c, 1, 0);
					}
					fclose(f);
					//open list error file and send contents of file byte by byte
					f = fopen("LISTerror.abcxyz", "r");
					while(!feof(f))
					{
						c = fgetc(f);
						send(transferSocket, &c, 1, 0);
					}
					fclose(f);
					c = '\r'; //Let transfer data socket know we are finished
					send(transferSocket, &c, 1, 0);
					memset(&message[0], 0, sizeof(message));
					close(transferSocket);
					//Let client know file transfer has completed
					strcpy(message, "226 File Transfer Complete\r\n");
						
				}
				else if(!strcmp(command, "PWD"))
				{
					//execute command to put pwd into outputfile named to avoid collision
					system("pwd>PWD.abcxyz");

					//Put contents read into message
					FILE *f = fopen("PWD.abcxyz", "r");
					message[0] = '2';
					message[1] = '5';
					message[2] = '7';
					message[3] = ' ';
					i = 4;
					while(!feof(f))
					{
						message[i++] = fgetc(f);
					}
					message[i - 1] = '\r';
					message[i] = '\n';
					fclose(f);
					//printf("pwd: %s", message);
					//send(ConnectFD, buffer, 1024, 0);
					//sendMessage(ConnectFD, buffer, 1024);
				}
				else if(!strcmp(command, "USER"))
				{
					strcpy(message, "230 User logged in, proceed.\r\n"); //placeholder
					//send(ConnectFD, message, sizeof(message), 0);
					//sendMessage(ConnectFD, message, sizeof(message));
				}
				else if(!strcmp(command, "SYST"))
				{
					strcpy(message, "502 Command not implemented.\r\n"); //Not implemented but needed
					//send(ConnectFD, message, sizeof(message), 0);
					//sendMessage(ConnectFD, message, sizeof(message));
				}
				else if(!strcmp(command, "FEAT"))
				{
					strcpy(message, "502 Command not implemented.\r\n"); //Not implemented but needed
					//send(ConnectFD, message, sizeof(message), 0);
					//sendMessage(ConnectFD, message, sizeof(message));
				}
				else if(!strcmp(command, "PORT"))
				{
					int host[4]; //House each byte of ip address sent
					unsigned char port[2]; //House each byte of port sent
					char ip[10]; //buffer to store combined ip address
					int ip_port; //buffer to store combined port 
					//Scan buffer for host and port
					sscanf(buffer, "PORT %d,%d,%d,%d,%d,%d", &host[0], &host[1], &host[2], &host[3], (int*)&port[0], (int*)&port[1]);
					sprintf(ip, "%d.%d.%d.%d", host[0], host[1], host[2], host[3]); //put host into ip
					//printf("IP: %s\n", ip);
					//put port into ip_port
					ip_port = port[0] * 256 + port[1];
					//printf("IP Port: %d\n", ip_port);

					//set sa 
					sa.sin_port = htons(ip_port);
					inet_pton(AF_INET, ip, &sa.sin_addr);
					transferSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
					//printf("transferSocket: %d \n", transferSocket);

					if(transferSocket == -1)
					{
						printf("Failed to create transfer socket\n");
					}

					//Connect to the transfer socket
					if(connect(transferSocket, (struct sockaddr *)&sa, (int)sizeof(struct sockaddr)) == -1)
					{
						printf("PORT failed\n");
						strcpy(message, "425 Can't open data connection.\r\n");
						//send(ConnectFD, message, sizeof(message), 0);
						//sendMessage(ConnectFD, message, sizeof(message));
					}
					else
					{
						//printf("PORT success\n");
						strcpy(message, "200 Command okay.\r\n");
						//send(ConnectFD, message, sizeof(message), 0);
						//sendMessage(ConnectFD, message, sizeof(message));
					}
				}
				else if(!strcmp(command, "TYPE"))
				{
					if(buffer[5] == 'I') // We only support image type
					{
						strcpy(message, "200 Command okay.\r\n");
						binary = 1;
					}
					else
					{
						buffer = strtok(NULL, "\r\n");
						strcpy(message, "504 Command not implemented for that parameter.\r\n");
						continue;
					}
				}
				else if(!strcmp(command, "MODE"))
				{
					if(buffer[5] == 'S') //We only support stream mode
					{
						strcpy(message, "200 Command okay.\r\n");
					}
					else
					{
						buffer = strtok(NULL, "\r\n");
						strcpy(message, "504 Command not implemented for that parameter.\r\n");
						continue;
					}
				}
				else if(!strcmp(command, "STRU"))
				{
					if(buffer[5] == 'F') //We only support file structure
					{
						strcpy(message, "200 Command okay.\r\n");
					}
					else
					{
						buffer = strtok(NULL, "\r\n");
						strcpy(message, "504 Command not implemented for that parameter.\r\n");
						continue;
					}
				}
				else if(!strcmp(command, "QUIT"))
				{
					quitLogical = 1; //To help break out of continuously listening to current client
					break;
				}
				else if(!strcmp(command, "RETR")) //Get file from server
				{
					if(binary) //must be in binary mode
					{
						//Let client know we are going to start transfering data to transfer socket
						strcpy(message, "125 Data connection already open; transfer starting.\n");
						//send(ConnectFD, message, 1024, 0);
						sendMessage(ConnectFD, message, strlen(message));
						memset(&message[0], 0, sizeof(message));
						//Open specified file and send contents of file byte by byte
						FILE *f = fopen(pathname, "r");
						char c;
						while(!feof(f)){
							c = fgetc(f);
							if(c == -1)
								break;
							send(transferSocket, &c, 1, 0);
						}
						fclose(f);
						close(transferSocket);
						//Let client know we are done
						strcpy(message, "226 Closing data connection. Requested file action successful.\r\n");
					}
					else //We are not in binary mode
						strcpy(message, "451 Requested action aborted: local error in processing\r\n");
				}
				else if(!strcmp(command, "STOR")) //Put file on server
				{
					if(binary) //Must be in binary mode
					{	
						//Let client know we are listening on transfer socket for data
						strcpy(message, "125 Data connection already open; transfer starting.\r\n");
						send(ConnectFD, message, strlen(message), 0);
						memset(&message[0], 0, sizeof(message));

						//Create empty file with name "pathname"... will overwrite
						//Ensures file is empty so we can append to it as we read from
						//transfer socket byte by byte
						FILE *f = fopen(pathname, "w");
						fclose(f);
						//Open up same, empty file but in append mode to be written byte by byte
						f = fopen(pathname, "a");

						//Listen on transfer socket port until EOF is read
						//We are only recieving data byte by byte and writing to file byte by byte
						char c = '\0';
						while(1)
						{
							recsize = recv(transferSocket, &c, 1, 0);

							if(recsize <= 0) 
								break;
							if(c == -1)
							{
								c = '\0';
								fwrite(&c, 1, 1, f);
								break;
							}
							if(c != '\r')
								fwrite(&c, 1, 1, f);
						}
						fclose(f);
						close(transferSocket);
						//Let client know we are finished
						strcpy(message, "226 Closing data connection. Requested file action successful.\r\n");
					}
					else //If not in binary mode
						strcpy(message, "451 Requested action aborted: local error in processing\r\n");
				}
				else if(!strcmp(command, "NOOP"))
				{
					strcpy(message, "200 Command okay.\r\n");
				}
				else //Command not implemented
				{
					strcpy(message, "502 \r\n");
				}

				buffer = strtok(NULL, "\r\n");
			}

			//if quit command sent, break out
			if(quitLogical)
			{
				quitLogical = 0;
				break;
			}

			//Done executing client commands sent and sends message back
			printf("message: %s", message);
			send(ConnectFD, &message[0], strlen(message), 0);
			//printf("next...\n");

		}

		/*if (shutdown(ConnectFD, SHUT_RDWR) == -1) 
		{
			printf("in shutdown\n");
			perror("shutdown failed");
			close(ConnectFD);
			close(SocketFD);
			exit(EXIT_FAILURE);
		}*/

		//We only get here if quit command is sent to break us out
		//Close socket that client was on and continue to wait for new client to connect
		close(ConnectFD);
		//printf("closed\n");
	}

	close(SocketFD);
	return EXIT_SUCCESS;  
}

//Helper funciton to send message byte by byte
void sendMessage(int socket, void * message, int len)
{

	if(!binary)
	{
		send(socket, message, len, 0);
		return;
	}

	int i;
	for(i = 0; i < len; i++)
	{
		send(socket, &((char*)message)[i], 1, 0);
		if(((char*)message)[i] == '\r')
			return;
	}
}