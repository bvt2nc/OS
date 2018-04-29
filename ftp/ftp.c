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

int main(void)
{
	struct sockaddr_in sa;
	char buffer[1024], command[5];
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

	if (bind(SocketFD, &sa, sizeof(sa)) == -1) 
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
			//printf("begin\n");
			recsize = recv(ConnectFD, &buffer[0], 1024, 0);
			sscanf(buffer, "%s", command);
			//printf("recv success \n");
			if (recsize < 0) 
			{
				fprintf(stderr, "%s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
			//printf("recsize: %d\n ", (int)recsize);
			sleep(1);
			printf("datagram: %.*s\n", (int)recsize, buffer);
			//printf("buffer: %s\n", buffer);
			printf("command: %s\n", command);

			/*for(i = 0; i < 5; i++)
			{
				command[i] = tolower(command[i]);
			}*/
			
			if(!strcmp(command, "LIST"))
			{
				system("ls -l > temp.txt");
				FILE *f = fopen("temp.txt", "r");
				i = 0;
				while(!feof(f))
					buffer[i++] = fgetc(f);
				buffer[i-1] = '\r';
				printf("list: %s \n", buffer);
				send(ConnectFD, buffer, 1024, 0);
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
				send(ConnectFD, buffer, 1024, 0);
			}
			else if(!strcmp(command, "USER"))
			{
				strcpy(message, "220 Recieved\r\n"); //placeholder
				send(ConnectFD, message, sizeof(message), 0);
			}
			else if(!strcmp(command, "SYST"))
			{
				strcpy(message, "502 Command not implemented.\r\n");
				send(ConnectFD, message, sizeof(message), 0);
			}
			else if(!strcmp(command, "FEAT"))
			{
				strcpy(message, "502 Command not implemented.\r\n");
				send(ConnectFD, message, sizeof(message), 0);
			}
			else if(!strcmp(command, "PORT"))
			{
				int host[4];
				int port[2];
				char ip[40];
				int ip_port;
				sscanf(buffer, "PORT %d,%d,%d,%d,%d,%d", &host[0], &host[1], &host[2], &host[3], &port[0], &port[1]);
				sprintf(ip, "%d.%d.%d.%d", host[0], host[1], host[2], host[3]);
				printf("IP: %s\n", ip);
				sa.sin_addr.s_addr = inet_addr(ip);
				ip_port = port[0] * 100 + port[1];
				printf("IP Port: %d\n", ip_port);
				sa.sin_port = htons(ip_port);
				transferSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

				if(transferSocket == -1)
				{
					printf("Failed to create transfer socket\n");
				}

				if (bind(transferSocket,(struct sockaddr *)&sa, sizeof sa) == -1) 
				{
					printf("bind failed\n");
					perror("bind failed");
					close(transferSocket);
					strcpy(message, "425 Can't open data connection.\r\n");
					send(ConnectFD, message, sizeof(message), 0);
				}

				if (listen(transferSocket, 10) == -1) 
				{
					printf("listen failed\n");
					perror("listen failed");
					close(transferSocket);
					strcpy(message, "425 Can't open data connection.\r\n");
					send(ConnectFD, message, sizeof(message), 0);
				}

				printf("PORT success\n");
					strcpy(message, "200 \r\n");
					printf("message: %s\n", message);
					send(ConnectFD, message, sizeof(message), 0);

				/*if(connect(transferSocket, (struct sockaddr *)&sa, (int)sizeof(struct sockaddr)) != 0)
				{
					printf("PORT failed\n");
					strcpy(message, "425 Can't open data connection.\r\n");
					send(ConnectFD, message, sizeof(message), 0);
				}
				else
				{
					printf("PORT success\n");
					strcpy(message, "200 Command okay.\r\n");
					send(ConnectFD, message, sizeof(message), 0);
				}*/
			}
			else if(!strcmp(command, "PASV"))
			{
				strcpy(message, "227 \r\n");
				printf("message: %s\n", message);
				sendsize = send(ConnectFD, (void*)message, sizeof(message), 0);	
			}
			else
			{
				strcpy(message, "502 \r\n");
				printf("message: %s\n", message);
				sendsize = send(ConnectFD, (void*)message, sizeof(message), 0);
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
