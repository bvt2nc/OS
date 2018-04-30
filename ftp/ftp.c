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

int binary = 0;

int main(void)
{
	struct sockaddr_in sa;
	char clientMsg[1024], command[5];
	char clientParse[1024];
	char message[1024] = "220 connected\r\n";
	ssize_t recsize, sendsize;
	socklen_t fromlen;
	int i;
	int SocketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	int transferSocket;
	if (SocketFD == -1) 
	{
		perror("cannot create socket");
		exit(EXIT_FAILURE);
	}

	memset(&sa, 0, sizeof sa);

	sa.sin_family = AF_INET;
	sa.sin_port = htons(1111);
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

	for (;;) 
	{
		int ConnectFD = accept(SocketFD, NULL, NULL);

		if (0 > ConnectFD) 
		{
			perror("accept failed");
			close(SocketFD);
			exit(EXIT_FAILURE);
		}

		printf("connectfd: %d\n", ConnectFD);

		sendsize = send(ConnectFD, (void*)message, sizeof(message), 0);
		if(sendsize < 0)
		{
			printf("Could not send 220\n");
			fprintf(stderr, "%s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		memset(&message[0], 0, sizeof(message));

		// perform read write operations ... 

		//recsize = recvfrom(ConnectFD, (void*)buffer, sizeof(buffer), 0, (struct sockaddr*)&sa, &fromlen);
		for(;;)
		{
			printf("waiting....\n");
			recsize = recv(ConnectFD, &clientMsg[0], 1024, 0);
			memset(&clientParse[0], 0, sizeof(clientParse));
			//memset(&message[0], 0, sizeof(message));
			
			for(i = 0; i < recsize; i++)
				clientParse[i] = clientMsg[i];
			clientParse[recsize] = '\0';

			printf("recsize: %d\n", (int)recsize);
			//printf("clientParse: %s\n", clientParse);
			printf("===================clientParse=================\n");
			for(i = 0; i < 1024; i++)
				printf("%c", clientParse[i]);
			printf("\n==============================================\n");

			char *buffer = strtok(clientParse, "\n\r");
			while(buffer != NULL)
			{
				//memset(&message[0], 0, 1024);
				sscanf(buffer, "%s", command);
				//printf("recv success \n");
				if (recsize < 0) 
				{
					fprintf(stderr, "%s\n", strerror(errno));
					exit(EXIT_FAILURE);
				}
				sleep(1);
				printf("======================DATAGRAM=================\n");
				printf("%s\n", buffer);
				printf("===============================================\n");
				//printf("buffer: %s\n", buffer);
				printf("command: %s\n", command);

				/*for(i = 0; i < 5; i++)
				{
					command[i] = tolower(command[i]);
				}*/
				
				if(!strcmp(command, "LIST"))
				{
					strcpy(message, "125 Transferring...\n");
					//send(ConnectFD, message, 1024, 0);
					sendMessage(ConnectFD, message, 1024);

					system("ls -l > temp.txt");
					i = 0;
					FILE *f = fopen("temp.txt", "r");
					while(!feof(f))
						message[i++] = fgetc(f);
					message[i-1] = '\r';
					printf("list: %s \n", message);
					if(fork() == 0)
					{
						printf("transferSocket: %d\n", transferSocket);
						send(transferSocket, message, 1024, 0);
						binary = 1;
						//sendMessage(transferSocket, message, 1024);
						binary = 0;
						exit(1);
					}
					else
					{
						wait(NULL);
						close(transferSocket);
						strcpy(message, "226 File Transfer Complete\r\n");
						printf("message: %s\n", message);
						//send(ConnectFD, message, 1024, 0);
						sendMessage(ConnectFD, message, 1024);
					}
						
				}
				else if(!strcmp(command, "PWD"))
				{
					//printf("in PWD\n");
					system("pwd>temp.txt");
					FILE *f = fopen("temp.txt", "r");
					buffer[0] = '2';
					buffer[1] = '5';
					buffer[2] = '7';
					buffer[3] = ' ';
					i = 4;
					while(!feof(f))
					{
						buffer[i++] = fgetc(f);
					}
					buffer[i - 1] = '\r';
					buffer[i] = '\n';
					fclose(f);
					printf("pwd: %s", buffer);
					//send(ConnectFD, buffer, 1024, 0);
					sendMessage(ConnectFD, buffer, 1024);
				}
				else if(!strcmp(command, "USER"))
				{
					strcpy(message, "220 Recieved\r\n"); //placeholder
					send(ConnectFD, message, sizeof(message), 0);
					//sendMessage(ConnectFD, message, sizeof(message));
				}
				else if(!strcmp(command, "SYST"))
				{
					strcpy(message, "502 Command not implemented.\r\n");
					//send(ConnectFD, message, sizeof(message), 0);
					sendMessage(ConnectFD, message, sizeof(message));
				}
				else if(!strcmp(command, "FEAT"))
				{
					strcpy(message, "502 Command not implemented.\r\n");
					//send(ConnectFD, message, sizeof(message), 0);
					sendMessage(ConnectFD, message, sizeof(message));
				}
				else if(!strcmp(command, "PORT"))
				{
					int host[4];
					unsigned char port[2];
					char ip[10];
					int ip_port;
					sscanf(buffer, "PORT %d,%d,%d,%d,%d,%d", &host[0], &host[1], &host[2], &host[3], (int*)&port[0], (int*)&port[1]);
					sprintf(ip, "%d.%d.%d.%d", host[0], host[1], host[2], host[3]);
					printf("IP: %s\n", ip);
					ip_port = port[0] * 256 + port[1];
					printf("IP Port: %d\n", ip_port);
					sa.sin_port = htons(ip_port);
					inet_pton(AF_INET, ip, &sa.sin_addr);
					transferSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
					printf("transferSocket: %d \n", transferSocket);

					if(transferSocket == -1)
					{
						printf("Failed to create transfer socket\n");
					}

					if(connect(transferSocket, (struct sockaddr *)&sa, (int)sizeof(struct sockaddr)) == -1)
					{
						printf("PORT failed\n");
						strcpy(message, "425 Can't open data connection.\r\n");
						//send(ConnectFD, message, sizeof(message), 0);
						sendMessage(ConnectFD, message, sizeof(message));
					}
					else
					{
						printf("PORT success\n");
						memset(&message[0], 0, 1024);
						strcpy(message, "200 Command okay.\r\n");
						//send(ConnectFD, message, sizeof(message), 0);
						sendMessage(ConnectFD, message, sizeof(message));
					}
				}
				else if(!strcmp(command, "TYPE"))
				{
					printf("buffer: %s\n", buffer);
					if(buffer[5] == 'I')
						strcpy(message, "200 \r\n");
					else
					{
						buffer = strtok(NULL, "\r\n");
						printf("recieved wrong parameter\n");
						continue;
						//strcpy(message, "504 \r\n");
					}
					printf("message: %s\n", message);
					//sendsize = send(ConnectFD, (void*)message, sizeof(message), 0);
					sendMessage(ConnectFD, (void*)message, sizeof(message));	
				}
				else
				{
					strcpy(message, "502 \r\n");
					printf("message: %s\n", message);
					//sendsize = send(ConnectFD, (void*)message, sizeof(message), 0);
					sendMessage(ConnectFD, (void*)message, sizeof(message));
				}

				/*strcpy(message, "220 Recieved\r\n");
				sendsize = send(ConnectFD, (void*)message, sizeof(message), 0);
				if(sendsize < 0)
				{
					printf("Message not sent\n");
					fprintf(stderr, "%s\n", strerror(errno));
					exit(EXIT_FAILURE);
				}
				memset(&message[0], 0, sizeof(message));*/

				buffer = strtok(NULL, "\r\n");
			}
			printf("next...\n");

		}

		if (shutdown(ConnectFD, SHUT_RDWR) == -1) 
		{
			printf("in shutdown\n");
			perror("shutdown failed");
			close(ConnectFD);
			close(SocketFD);
			exit(EXIT_FAILURE);
		}
		close(ConnectFD);
		printf("closed\n");
	}

	close(SocketFD);
	return EXIT_SUCCESS;  
}

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