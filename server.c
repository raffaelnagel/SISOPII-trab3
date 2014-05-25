#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define FALSE 0
#define TRUE 1
#define MESSAGESIZE 256
#define USERNAMESIZE 256
#define PORT 4000
#define MAXCONNECTIONS 5

typedef struct{
	int in;
} param;

typedef struct{	
	int id;
	char status[7];
	char* username;
	int socket;
	pthread_t thread;
} Client;

Client* clients;
int MaxClientsNumber = 400;
int Clients_Count;
int CHAT_ONLINE;
int color = 31;

pthread_attr_t pthread_custom_attr;
pthread_t adminThread;
pthread_t acceptLoopThread;

int sockfd;
struct sockaddr_in server_address;

void initSocket();
void acceptConnection(int clientId);
void getUsername(int clientId);
void getChatTime(char* buffer);
void toLower(char* result, char* string);
void readClientMessages(int clientId);
void sendToAll(char* message);
void* serverCommand(void* args);
void DisconnectUser(int clientId);
void* acceptConnection_LOOP(void* args);
void* worker(void* args);

int main(int argc, char const *argv[]){
	
	int i;

	initSocket();

	// allocating memory for clients' array
	clients = (Client*)calloc(MaxClientsNumber,sizeof(Client));

	// initializing clients
	for(i = 0; i < MaxClientsNumber; i++){
		strncpy(clients[i].status, "OFFLINE", 7);
		clients[i].id = i;
		clients[i].username = (char*)calloc(USERNAMESIZE,sizeof(char));		
	}

	CHAT_ONLINE = TRUE;

	system("clear");
	fprintf(stderr,"CHAT OPENED!\n");
	fprintf(stderr,"-- 'close' to end the chat\n\n");

	pthread_create(&adminThread, &pthread_custom_attr, serverCommand, (void*)0);
	pthread_create(&acceptLoopThread, &pthread_custom_attr, acceptConnection_LOOP, (void*)0);

	
	while(CHAT_ONLINE){
		//loop
	}

	for(i = 0; i < Clients_Count; i++){
		if(strcmp(clients[i].status,"ONLINE") == 0){
			DisconnectUser(i);
		}
	}

	close(sockfd);

	return 0;
}

void initSocket(){	
	
	
	//SOCKET CREATION
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){ 
        printf("ERROR opening socket");
        exit(0);
	}

	int optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	server_address.sin_addr.s_addr = INADDR_ANY;
	bzero(&(server_address.sin_zero), 8);

	// BINDING
	if (bind(sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0){ 
		printf("ERROR on binding\n");		
		exit(0);
	}	
}

void acceptConnection(int clientId){
	int newsockfd;
	socklen_t client_length;
	struct sockaddr_in client_address;

	listen(sockfd, MAXCONNECTIONS);

	client_length = sizeof(struct sockaddr_in);

	if ((newsockfd = accept(sockfd, (struct sockaddr*) &client_address, &client_length)) == -1){
		printf("ERROR on accept\n");
		exit(0);
	}
	
	clients[clientId].socket = newsockfd;
	strncpy(clients[clientId].status, "ONLINE", 7);
}

void getUsername(int clientId){
	
	int n;
	char buffer[USERNAMESIZE];
	bzero(buffer, USERNAMESIZE);
	//printf("DEBUG A1\n");
	/* read from the socket */
	n = read(clients[clientId].socket, buffer, USERNAMESIZE);
	if(n < 0){
		fprintf(stderr,"ERROR reading from socket\n");
		exit(0);
	}
	//printf("DEBUG A2\n");
        
	sprintf(clients[clientId].username,"%c[%d;%dm%s%c[%dm",27,1,color,buffer,27,0);
	//sprintf(clients[clientId].username,"%s",buffer);
	//printf("DEBUG A3\n");
	color = 31 + (color - 31 + 1)%6;
}

void getChatTime(char* buffer){
	time_t time_now = time(0);
    struct tm  newTime;
    newTime = *localtime(&time_now);

    sprintf(buffer,"%2d:%2d:%2d", newTime.tm_hour, newTime.tm_min, newTime.tm_sec);
}

void toLower(char* result, char* string){
	int i;
	for(i = 0; i < strlen(string); i++)	{
		result[i] = tolower(string[i]);
	}
}

void readClientMessages(int clientId){
	
	int n;
	char message[MESSAGESIZE];

	while(CHAT_ONLINE){
		
		char messageTime[80];
		getChatTime(messageTime);

		bzero(message, MESSAGESIZE);
	
		n = read(clients[clientId].socket, message, MESSAGESIZE);	
		if(n < 0){
			fprintf(stderr,"ERROR reading from socket\n");
			exit(0);
		}

		char* lowerCaseMSG = (char*)calloc(strlen(message),sizeof(char));
		toLower(lowerCaseMSG,message);
		
		//message = (char)lowerCaseMSG;
		
		if(strcmp(lowerCaseMSG,"/logout") == 0){			
			char logoutMessage[MESSAGESIZE];
			sprintf(logoutMessage,"\n%s	>> %s is now offline <<\n\n", messageTime, clients[clientId].username);

			strncpy(clients[clientId].status, "OFFLINE", 7);
			close(clients[clientId].socket);
			sendToAll(logoutMessage);

			return;
		}
		else{
			char clientMessage[MESSAGESIZE];
			sprintf(clientMessage, "%s	%s says: %s\n", messageTime, clients[clientId].username, message);
			sendToAll(clientMessage);
		}
	}
}

void sendToAll(char* message){
	int i, n;

	for(i = 0; i < MaxClientsNumber; i++){
		Client newClient = clients[i];

		if(strcmp(newClient.status, "ONLINE") == 0){
			n = write(clients[i].socket, message, strlen(message));
			if(n < 0){
				fprintf(stderr,"ERROR writing to socket");
				exit(0);
			}
		}
	}
}

void* serverCommand(void* args){
	
	char command[100];
	bzero(command,100);

	fprintf(stderr,"> ");

	fgets(command,100,stdin);

	while(CHAT_ONLINE){
		
		if(strcmp(command,"close\n") == 0){
			fprintf(stderr, "\nClosing chat...\n");
			CHAT_ONLINE = FALSE;
			fprintf(stderr, "Chat is now offline.\n");
		}
		else{
			fprintf(stderr,"Unknow Command\n");
		}

		fprintf(stderr,"> ");
		fgets(command,100,stdin);
	}
	return;
}

void DisconnectUser(int clientId){
	
	strncpy(clients[clientId].status, "OFFLINE", 7);

	int n = write(clients[clientId].socket, "KILL", strlen("KILL"));
	if(n < 0){
		fprintf(stderr,"ERROR writing to socket");
		exit(0);
	}

	close(clients[clientId].socket);
}

void* acceptConnection_LOOP(void* args){
	
	Clients_Count = 0;

	while(CHAT_ONLINE){
		Clients_Count++;

		acceptConnection(Clients_Count-1);
		pthread_create(&(clients[Clients_Count-1].thread), &pthread_custom_attr, worker, &(clients[Clients_Count-1].id));
	}
}

void* worker(void* args){
	
	param* p = (param*)args;
	int clientId = p->in;
	//printf("DEBUG username= %s\n", clients[clientId].username);
	getUsername(clientId);
	//printf("DEBUG 1\n");	
	char loginMessage[MESSAGESIZE], messageTime[MESSAGESIZE];
	getChatTime(messageTime);
	sprintf(loginMessage,"\n%s	>> %s enters the room <<\n\n",messageTime, clients[clientId].username);
	sendToAll(loginMessage);

	readClientMessages(clientId);
}








