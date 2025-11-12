#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>

#define SERV_PORT 2345
const char* req_mess = "req";
const char* err_mess = "err";

#define MAX_CLIENTS 10
struct sockaddr_in clients[MAX_CLIENTS];

int config_sd() {
	// apri socket
	int sd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sd < 0) {
		perror("Creazione socket fallita");
		return -1;
	}

	// configura indirizzo 
	struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(SERV_PORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;	
	
	// associa indirizzo a socket
	if(bind(sd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("Bind socket fallita");
		close(sd);
		return -1;
	}

	return sd;
}

int regist(struct sockaddr_in in) {
	printf("Ricevuta richiesta, sin_family: %d, sin_addr: %d, sin_port: %d\n",
	       in.sin_family, in.sin_addr.s_addr, in.sin_port);
	
	// scorri buffer client per spazio libero
	for(int i = 0; i < MAX_CLIENTS; i++) {
		if(clients[i].sin_family == 0) {
			clients[i] = in;
			printf("Registrato il client %d, sin_family: %d, sin_addr: %d, sin_port: %d\n",
			       i, clients[i].sin_family, clients[i].sin_addr.s_addr, clients[i].sin_port);
			return 1;
		}
	}

	printf("Client scartato per buffer pieno\n");
	// pieno, non fare nulla
	return 0;
}

void unregist(int i) {
	printf("Deregistrato il client %d, sin_family: %d, sin_addr: %d, sin_port: %d\n",
	       i, clients[i].sin_family, clients[i].sin_addr.s_addr, clients[i].sin_port);
	clients[i].sin_family = 0;			
}

void serialize_time(uint32_t* buf) {
	// ottieni il tempo
	time_t raw_time;
	struct tm* local_time;
	time(&raw_time);
	local_time = localtime(&raw_time);

	buf[0] = htonl((uint32_t) local_time->tm_sec);
	buf[1] = htonl((uint32_t) local_time->tm_min);
	buf[2] = htonl((uint32_t) local_time->tm_hour);
	buf[3] = htonl((uint32_t) local_time->tm_mday);
	buf[4] = htonl((uint32_t) local_time->tm_mon);
	buf[5] = htonl((uint32_t) 1900 + local_time->tm_year);
}

void serve(int sd) {
	// inizializza buffer per recvfrom
	char buf[strlen(req_mess) + 1];
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	// ricevi richieste finché ce ne sono
	errno = 0;
	while(1) {
		int ret = recvfrom(
			sd,
			buf,	
			sizeof(buf), 
			MSG_DONTWAIT, 
			(struct sockaddr*) &client_addr, 
			&client_addr_len
		);
		if(ret < 0) {
			// se avrebbe bloccato non ci sono più richieste, esci
			if(errno == EAGAIN) break;

			perror("Ricezione fallita");
			continue;
		}

		if(strcmp(buf, req_mess) == 0) {
			// richiesta ben formata, registra
			int avail = regist(client_addr);

			// se non hai potuto registrare, avverti
			if(!avail) {
				ret = sendto(
					sd,
					err_mess,
					sizeof(err_mess) + 1,
					0,
					(struct sockaddr*) &client_addr,
					sizeof(client_addr)
				);
			}
		}
	}

	// invia tempo
	for(int i = 0; i < MAX_CLIENTS; i++) {	
		if(clients[i].sin_family == 0) continue;

		// rileva se ha chiuso
		int ret = recvfrom(
			sd,
			buf,	
			sizeof(buf), 
			MSG_DONTWAIT, 
			(struct sockaddr*) &client_addr, 
			&client_addr_len
		);
		if(ret < 0 && errno != EAGAIN) {
			perror("Ricezione fallita");
		}
		if(strcmp(buf, err_mess) == 0) {
			unregist(i);
			continue;
		}

		// serializza		
		uint32_t time[6];
		serialize_time(time);

		ret = sendto(
			sd, 
			(void*) time,
			sizeof(time),
			0,
			(struct sockaddr*) &clients[i], 
			sizeof(clients[i])
		);
		if(ret < 0) {
			perror("Invio fallito");
		}
	}

	// riposa
	sleep(1);
}

int main() {
	printf("--- Time server v0.0 ---\n");

	// resetta buffer client
	for(int i = 0; i < MAX_CLIENTS; i++) {
		clients[i].sin_family = 0; // segnala indirizzo nullo
	}

	// configura socket e indirizzo server
	int sd = config_sd();
	if(sd == -1) return 1;

	// servi client
	while(1) {
		serve(sd);
	}

	// chiudi e esci
	close(sd);
	return 0;
}
