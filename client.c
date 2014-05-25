#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define FALSE 0
#define TRUE 1
#define CHATLOGSIZE 1048576
#define BUFFERSIZE 256
#define PORT 4000

pthread_t thread_writer;
pthread_t thread_reader;
pthread_attr_t pthread_custom_attr;

int sockfd;
char chatLog[CHATLOGSIZE];
char command[BUFFERSIZE];
char status[7];

pthread_mutex_t mutexRefresh;
pthread_mutex_t mutexLogout;
pthread_mutex_t mutexReadMessage;

void initSocket();
void userLogin();
void* readServerMessage(void* args);
void* sendMessageToServer(void* args);
void refresh();

int main(int argc, char const *argv[]){
	initSocket();

	pthread_mutex_init(&mutexRefresh,NULL);
	pthread_mutex_init(&mutexLogout,NULL);

	sprintf(chatLog,"");

	userLogin();

	printf("\nuser logged\n");

	pthread_create(&thread_writer, &pthread_custom_attr, sendMessageToServer, (void*)0);
	pthread_create(&thread_reader, &pthread_custom_attr, readServerMessage, (void*)0);

	pthread_join(thread_writer, NULL);
	pthread_join(thread_reader, NULL);

	system("clear");
	fprintf(stderr,"You are now offline!\n");

	return 0;
}

void initSocket(){
	struct sockaddr_in server_address;
	struct hostent *server;

	server = gethostbyname("0");
	if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("ERROR opening socket\n");
        exit(0);
	}

	server_address.sin_family = AF_INET;     
	server_address.sin_port = htons(PORT);    
	server_address.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(server_address.sin_zero), 8);

	if (connect(sockfd,(struct sockaddr *) &server_address,sizeof(server_address)) < 0){ 
        printf("ERROR connecting\n");
        exit(0);
	}
}

void userLogin(){

	char buffer[BUFFERSIZE];
	int n;

	fprintf(stderr,"username: ");
	bzero(buffer, BUFFERSIZE);
	fgets(buffer, BUFFERSIZE, stdin);

	buffer[strlen(buffer) -1] = '\0';

	//printf("DEBUG %s\n", buffer);

	/* write in the socket */
	n = write(sockfd, buffer, strlen(buffer));
	if(n < 0){
		fprintf(stderr,"ERROR writing to socket\n");
		exit(0);
	}
	strncpy(status, "ONLINE", 7);
}

void* sendMessageToServer(void* args){

	int n;

	while(strcmp(status, "ONLINE") == 0){

		refresh();

    	bzero(command, BUFFERSIZE);
    	fgets(command, BUFFERSIZE, stdin);

    	command[strlen(command) -1] = '\0';
    	
		/* write in the socket */
		n = write(sockfd, command, strlen(command));
    	if (n < 0){
			fprintf(stderr,"ERROR writing to socket\n");
			exit(0);
		}

		if(strcmp(command,"/logout") == 0){
    		close(sockfd);
    		strncpy(status, "OFFLINE", 7);
    		return;
    	}
		bzero(command, BUFFERSIZE);
	}
	return;
}

void* readServerMessage(void* args){

	char buffer[BUFFERSIZE];
	int n;

	while(strcmp(status, "ONLINE") == 0){

    	bzero(buffer,BUFFERSIZE);

		/* read from the socket */
    	n = read(sockfd, buffer, BUFFERSIZE);
    	if (n < 0){
			fprintf(stderr,"ERROR reading from socket\n");
			exit(0);
		}

		if(strcmp(buffer,"KILL") == 0){
			pthread_mutex_lock(&mutexLogout);

			close(sockfd);

			strncpy(status, "OFFLINE", 7);
			system("clear");
			fprintf(stderr,"You are now offline.\n");

			pthread_mutex_unlock(&mutexLogout);

			exit(0);
		}
		else{
			strcat(chatLog,buffer);
			//strcat(chatLog,typingBuffer);
			//printf("DEBUG READ\n");
			refresh();
		}
	}
	return;
}

void refresh(){

	pthread_mutex_lock(&mutexRefresh);

	system("clear");

	fprintf(stderr,"Welcome! You are now online!\n");
	fprintf(stderr,"-- '/logout' to exit the chat\n\n");
	fprintf(stderr,"-- '/create' to create a room\n\n");
	fprintf(stderr,"-- '/join' to enter the room\n\n");
	fprintf(stderr,"-- '/leave' to exit the room\n\n");
	fprintf(stderr,"-- '/change' to change your nickname\n\n");
    fprintf(stderr,"%s\nType your messages here: ",chatLog);

    pthread_mutex_unlock(&mutexRefresh);
}
