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

int config_ld() {
	// apri socket
	int ld = socket(AF_INET, SOCK_STREAM, 0);
	if(ld < 0) {
		perror("Creazione socket di ascolto fallita");
		return -1;
	}

	// configura indirizzo 
	struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(SERV_PORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;	
	
	// associa indirizzo a socket
	if(bind(ld, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("Bind socket di ascolto fallita");
		close(ld);
		return -1;
	}

	// ascolta su socket
	if(listen(ld, 10) < 0) {
		perror("Ascolto su socket di ascolto fallito");
		close(ld);
		return -1;
	}

	return ld;
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

int serve(int cd) {
	// inizializza buffer per recv
	char buf[strlen(req_mess) + 1];

	// ricevi richiesta
	int ret = recv(
		cd,
		buf,	
		sizeof(buf), 
		MSG_WAITALL
	);
	if(ret < 0) {
		perror("Ricezione fallita");
		return 1;
	}
	if(ret == 0) return 0;

	if(strcmp(buf, req_mess) == 0) {
		// serializza		
		uint32_t time[6];
		serialize_time(time);

		ret = send(
			cd, 
			(void*) time,
			sizeof(time),
			0
		);
		if(ret < 0) {
			perror("Invio fallito");
		}
	}

	return 1;
}

int main() {
	printf("--- Time server v0.0 ---\n");

	// configura socket e indirizzo server
	int ld = config_ld();
	if(ld == -1) return 1;

	// configura master set
	fd_set master_set;
	FD_ZERO(&master_set);
	FD_SET(ld, &master_set);
	int fdmax = ld;

	// configura read set
	fd_set read_set;
	FD_ZERO(&read_set);

	while(1) {
		read_set = master_set;
		select(fdmax + 1, &read_set, NULL, NULL, NULL);
		for(int i = 0; i < fdmax + 1; i++) {
                        if(!FD_ISSET(i, &read_set)) continue;

                        if(i == ld) {
                                // è il listener, collega un nuovo client
                                struct sockaddr_in client_addr;
                                int client_len = sizeof(client_addr);

                                int cd = accept(i, (struct sockaddr*) &client_addr, &client_len);
                                if(cd < 0) {
                                        perror("Accept fallita");
                                        continue;
                                }

                                char addr_str[INET_ADDRSTRLEN];
                                inet_ntop(AF_INET, &(client_addr.sin_addr), addr_str, INET_ADDRSTRLEN);

                                printf("Connesso a client %s\n", addr_str);

                                FD_SET(cd, &master_set);
                                if(cd > fdmax) fdmax = cd;
                        } else {
                                // è un client
                        	if(!serve(i)) FD_CLR(i, &master_set);
			}

                }
	}	

	return 0;
}
