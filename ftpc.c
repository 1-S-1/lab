#include <stdio.h>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dirent.h>
#include <stdio.h>
#include <string.h>

#define MAXFILE 100
#define FILENAME 100



void performGET(char *file_name,int socket_desc);
void performPUT(char *file_name,int socket_desc);
int SendFileOverSocket(int socket_desc, char* file_name);


int main(int argc , char **argv)
{


	if(argc!=3){
		printf("Invalid arguments\n");
		return 0;
	}
	int socket_desc;
	struct sockaddr_in server;
	char request_msg[BUFSIZ], reply_msg[BUFSIZ], file_name[BUFSIZ];
	
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1)
	{
		perror("Could not create socket");
		return 1;
	}

	char SERVER_IP[BUFSIZ];
	int SERVER_PORT;
	strcpy(SERVER_IP,argv[1]);
	SERVER_PORT=atoi(argv[2]);
	server.sin_addr.s_addr = inet_addr(SERVER_IP);
	server.sin_family = AF_INET;
	server.sin_port = htons(SERVER_PORT);

	// Connect to server
	if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		perror("Connection failed");
		return 1;
	}

	int choice = 0;
	while(1)
	{
		printf("Enter a choice:\n1- GET\n2- PUT\n3- EXIT\n");
		scanf("%d", &choice);
		switch(choice)
		{
			case 1:
				printf("Enter file_name to get: ");
				scanf("%s", file_name);
				performGET(file_name,socket_desc);
				break;
			case 2:
				printf("Enter file_name to put: ");
				scanf("%s", file_name);
				performPUT(file_name,socket_desc);
				break;
			case 3:
				strcpy(request_msg,"EXIT");
				write(socket_desc, request_msg, strlen(request_msg));	
				return 0;
			default: 
				printf("Incorrect command\n");
		}
	}
	return 0;
}

int SendFileOverSocket(int socket_desc, char* file_name)
{
	struct stat	obj;
	int file_desc, file_size;

	stat(file_name, &obj);
	file_desc = open(file_name, O_RDONLY);
	file_size = obj.st_size;
	send(socket_desc, &file_size, sizeof(int), 0);
	sendfile(socket_desc, file_desc, NULL, file_size);

	return 1;
}

void performGET(char *file_name,int socket_desc){
	char request_msg[BUFSIZ], reply_msg[BUFSIZ];
	int file_size;
	char *data;
	int t;
	if( access( file_name, F_OK ) != -1 )
	{
		int abortflag = 0;
		printf("File already exists locally. Press 1 to overwrite. Press any other key to abort.\n");
		scanf("%d", &abortflag);
		if(abortflag!=1)
			return;
	}
	// Get a file from server
	strcpy(request_msg, "GET ");
	strcat(request_msg, file_name);
	write(socket_desc, request_msg, strlen(request_msg));
	recv(socket_desc, reply_msg, 2, 0);
	reply_msg[2] = '\0';
	printf("%s\n", reply_msg);
	if (strcmp(reply_msg, "OK") == 0)
	{
	
		// File present at server.Start receiving the file and storing locally.
		printf("Recieving data\n");
	
		recv(socket_desc, &file_size, sizeof(int), 0);
		data = malloc(file_size+1);
		FILE *fp = fopen(file_name, "w"); 
		t = recv(socket_desc, data, file_size, 0);
		data[t] = '\0';
		fputs(data, fp);
		fclose(fp);
		printf("File %s recieved with size %d \n", file_name,t);
	}
	else
	{
		printf("File doesn't exist at server.ABORTING.\n");
	}
}

void performPUT(char *file_name,int socket_desc)
{
	int	file_size, file_desc,c,t;
	char *data;
	char request_msg[BUFSIZ], reply_msg[BUFSIZ],client_response[2];
	// Get a file from server
	strcpy(request_msg, "PUT ");
	strcat(request_msg, file_name);
	printf("Trying to PUT %s to server. \n",file_name );
	if (access(file_name, F_OK) != -1)
	{
		// Sending PUT request to server.
		write(socket_desc, request_msg, strlen(request_msg));
		t = recv(socket_desc, reply_msg, BUFSIZ, 0);
		reply_msg[t]='\0';
		if (strcmp(reply_msg, "OK") == 0)
		{
			// Everything if fine and send file
			SendFileOverSocket(socket_desc, file_name);
		}
		else if(strcmp(reply_msg, "FP") == 0)
		{
			// File present at server.
			printf("File exists in server. Do you wan't to overwrite? 1/0\n");
			scanf("%d", &c);
			if(c)
			{
				// User says yes to overwrite. Send Y and then data
				printf("Overwriting %s\n",file_name );
				strcpy(client_response, "Y");
				write(socket_desc, client_response, strlen(client_response));
				SendFileOverSocket(socket_desc, file_name);
			}
			else
			{
				printf("Not sending %s file to server\n",file_name);
				// User says No to overwrite. Send N and exit
				strcpy(client_response, "N");
				write(socket_desc, client_response, strlen(client_response));
				return;
			}
		}
		else{
			// Server can't create file.
			printf("Server can't create file...\n");
		}
	}
	else
	{
		// File not found locally hence abort.
		printf("File not found locally...\n");
		return;
	}
}


