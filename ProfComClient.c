#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#define MAX_PASSWORD_LENGTH 256
#define KEY 0x45
struct Packet{

	char username[256];
	int username_length;
	int password_length;
	char password[MAX_PASSWORD_LENGTH];
	bool authentication_status;
	int message_length;
	char* message;
};

bool authenticate(int fd){

	struct Packet login;
	int valread;
	printf("Please enter your username: ");
	scanf("%s",login.username);
	printf("Please enter your password: ");
	scanf("%s", login.password);

	login.username_length = strlen(login.username);
	login.password_length = strlen(login.password);

	for(int i = 0; i < login.password_length; i++){
		login.password[i] ^= KEY;
	}

	send(fd, &login.username_length, sizeof(login.username_length), 0);
	send(fd, login.username, login.username_length,0);
	send(fd, &login.password_length, sizeof(login.password_length),0);
	send(fd, login.password, login.password_length,0);
	printf("Username and Password Sent\n\n");
	if((valread = read(fd, &login.authentication_status, sizeof(login.authentication_status))) <= 0){
		perror("Failed to read authentication status");
		exit(EXIT_FAILURE);
	}

	if(!login.authentication_status){
		printf("Authentication Failed\n");
	}

	return login.authentication_status;
}

void data_transfer(int fd){

	struct Packet data;
	char buffer[2048];
	char* message;
	int i = 0;
	char c;
	printf("\nPlease enter the message you would like to send when you are finished type ^ and press enter: \n");

	while((c = getc(stdin)) != '^'){
		buffer[i] = c;
		i++;
	}
	buffer[i] = '\0';
	data.message = buffer;

	data.message_length = strlen(data.message);

	for(int j = 0; j < data.message_length; j++){
		data.message[j] ^= KEY;
	}

	send(fd, &data.message_length, sizeof(data.message_length), 0);
	send(fd, data.message, data.message_length, 0);

}

int main(int argc, char const* argv[]){

	int status,valread, client_fd,messages;
	struct sockaddr_in serv_addr;

	if((client_fd = socket(AF_INET, SOCK_STREAM,0)) < 0){

		perror("Socket creation error");
		exit(EXIT_FAILURE);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(8080);

	if(inet_pton(AF_INET, "65.0.0.2", &serv_addr.sin_addr) <= 0){
		perror("Invalid address / Address not supported");
		exit(EXIT_FAILURE);
	}

	if ((status = connect(client_fd, (struct sockaddr*)&serv_addr,sizeof(serv_addr))) < 0){

		perror("Connection failed");
		exit(EXIT_FAILURE);
	}


	while(!authenticate(client_fd));

	printf("Authentication Success\n\n");

	printf("Please enter the amount of messages you would like to send: ");
	scanf("%d",&messages);
	send(client_fd, &messages, sizeof(messages) ,0);

	for(int i=0; i < messages; i++){
		data_transfer(client_fd);
	}

	close(client_fd);

	return 0;

}
