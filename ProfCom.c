#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <bcrypt.h>
#include <stdbool.h>
#define MAXPASSWORDLENGTH 256
#define KEY 0x45;
struct Packet{

	int username_length;
	char username[256];
	int password_length;
	char password[MAXPASSWORDLENGTH];
	bool authentication_status;
	int message_length;
	char message[2048];
};

struct User{

	char* username;
	char* password_hash;

};

struct User ProfessorX = {
	.username = "ProfessorX",
	.password_hash = "$2a$12$KUQTyz0XzLVtTexrK2rpEeYHmT8L0OoAORW/jd4e0Uo3yUnR9ov2G"
};
struct User AllAmericanGirl = {
	.username= "All American Girl",
	.password_hash = ""
};

struct Results{
	bool authentication_status;
	char authenticated_user[256];
};

struct Results read_authenticate(int fd){

	ssize_t valread;
	int ret;
	struct Packet login;
	struct Results AuthResult;
	login.authentication_status = false;

	if((valread = read(fd, &login.username_length, sizeof(login.username_length))) <= 0) {
		perror("Failed to read username length");
		exit(EXIT_FAILURE);
	}

	if((valread = read(fd, login.username, login.username_length)) <= 0){
		perror("Failed to read username");
		exit(EXIT_FAILURE);
	}

	if((valread = read(fd, &login.password_length, sizeof(login.password_length))) <= 0){
		perror("Failed to read password length");
		exit(EXIT_FAILURE);
	}

	if((valread = read(fd, login.password, login.password_length)) <= 0){
		perror("Failed to read password");
		exit(EXIT_FAILURE);
	}

	for(int i =0; i < login.password_length; i++){
		login.password[i] ^= KEY;
	}

	if(strcmp(login.username, ProfessorX.username) == 0){
		ret = bcrypt_checkpw(login.password, ProfessorX.password_hash);
		if(ret != 0){
			printf("Failed login by: %s\n", login.username);
			perror("Passwords did not match");
			send(fd, &login.authentication_status, sizeof(login.authentication_status),0);
			return AuthResult;
		}else if (strcmp(login.username, AllAmericanGirl.username) == 0){
			ret = bcrypt_checkpw(login.password, AllAmericanGirl.password_hash);
				if(ret != 0){
					printf("Failed login by: %s\n", login.username);
					perror("Passwords did not match");
					send(fd, &login.authentication_status, sizeof(login.authentication_status),0);
					return AuthResult;
				}
		}else{
			printf("Failed login by: %s\n", login.username);
			perror("Passwords did not match");
			send(fd, &login.authentication_status, sizeof(login.authentication_status),0);
			return AuthResult;
		}
	}
	printf("Authentication Passed\n");
	printf("%s has successfully logged in\n", login.username);

	login.authentication_status = true;
	AuthResult.authentication_status = true;
	strcpy(AuthResult.authenticated_user, login.username);
	send(fd, &login.authentication_status, sizeof(login.authentication_status),0);
	return AuthResult;

}

void read_message(int fd, struct Results auth_user){

	ssize_t valread;
	struct Packet data_transfer;
	memset(data_transfer.message, 0, sizeof(data_transfer.message));
	if((valread = read(fd, &data_transfer.message_length, sizeof(data_transfer.message_length))) <= 0){
		perror("Unable to read message length");
		exit(EXIT_FAILURE);
	}

	if((valread = read(fd, data_transfer.message, data_transfer.message_length)) <= 0){
		perror("Unable to read message");
		exit(EXIT_FAILURE);
	}

	for( int i = 0 ; i < data_transfer.message_length; i++){
		data_transfer.message[i] ^= KEY;
	}

	printf("\n\nMessage from %s: %s\n", auth_user.authenticated_user, data_transfer.message);

}

int main(int argc, char *const argv[]){

	int server_fd, new_socket, messages;
	ssize_t valread;
	struct sockaddr_in address;
	struct Results user;
	int opt = 1;
	socklen_t addrlen = sizeof(address);

		if((server_fd = socket(AF_INET, SOCK_STREAM,0)) < 0){
			perror("Socket Failed");
			exit(EXIT_FAILURE);
		}

		if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
			perror("setsockopt");
			exit(EXIT_FAILURE);
		}

		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons(8080);

		if(bind(server_fd , (struct sockaddr*)&address,sizeof(address)) < 0){

			perror("bind failed");
			exit(EXIT_FAILURE);

		}

		if(listen(server_fd, 3) < 0){

			perror("listen");
			exit(EXIT_FAILURE);
		}
		while(true){
			if((new_socket = accept(server_fd, (struct sockaddr*)&address,&addrlen)) < 0){

				perror("accept");
				exit(EXIT_FAILURE);
			}

			user = read_authenticate(new_socket);

			while (user.authentication_status == 0){

				user = read_authenticate(new_socket);

			}

			if((valread = read(new_socket, &messages, sizeof(messages))) <= 0){
				perror("Unable to read amount of messages");
				exit(EXIT_FAILURE);
			}

			for(int i = 0; i < messages; i++){
				read_message(new_socket, user);
			}
		}

	close(server_fd);

	return 0;

}
