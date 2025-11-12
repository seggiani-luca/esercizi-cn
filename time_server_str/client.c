#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>

#define SERV_ADDR "127.0.0.1"
#define SERV_PORT 2345
const char* req_mess = "req";
const char* err_mess = "err";
struct sockaddr_in serv_addr;

int should_quit = 0;

void int_handle(int sig) {
	should_quit = 1;
}

int config_cd() {
	// apri socket
	int cd = socket(AF_INET, SOCK_DGRAM, 0);
	if(cd < 0) {
		perror("Creazione socket fallita");
		return -1;
	}

	// configura indirizzo 
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(SERV_PORT);
        inet_pton(AF_INET, SERV_ADDR, &serv_addr.sin_addr);

	return cd;
}

int get_time(int cd) {	
	char time[20];
	struct sockaddr_in temp_addr = {0};
	socklen_t temp_addr_len;
	// cicla finché il server non risponde 
	do {	
		int ret = recvfrom(
			cd,
			(void*) time,
			sizeof(time),
			0,
			(struct sockaddr*) &temp_addr,
			&temp_addr_len
		);
		if(ret < 0) {
			perror("Ricezione fallita");
		}
		if(strcmp((char*) time, err_mess) == 0) {
			printf("Errore lato server\n");
			return 0;
		}
	} while(temp_addr.sin_port != serv_addr.sin_port ||
	        temp_addr.sin_addr.s_addr != serv_addr.sin_addr.s_addr);

	// deserializza	
	printf("%s\n", time);

	return 1;
}

int main() {
	// imposta handler
	signal(SIGINT, int_handle);
	signal(SIGTERM, int_handle);

	// configura socket e indirizzo server
	int cd = config_cd();
	if(cd == -1) return 1;

	// registrati
	printf("Mi registro al server\n");
	int ret = sendto(
		cd, 
		req_mess, 
		strlen(req_mess) + 1, 
		0, 
		(struct sockaddr*) &serv_addr, 
		sizeof(serv_addr)
	);
	if(ret < 0) {
		perror("Registrazione fallita");
		close(cd);
		exit(1);
	}

	// ottieni tempo
	while(get_time(cd)) {
		if(should_quit) {
			// deregistrati
			printf("Mi deregistro dal server\n");
			ret = sendto(
				cd,
				err_mess, 
				strlen(err_mess) + 1, 
				0, 
				(struct sockaddr*) &serv_addr, 
				sizeof(serv_addr)
			);
			if(ret < 0) {
				perror("Deregistrazione fallita");
				continue;
			}

			break;
		}
	}

	close(cd);
	return 0;
}
