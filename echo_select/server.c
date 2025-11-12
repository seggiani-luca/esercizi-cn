#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "utils.h"

#define SERV_PORT "1234"

int configure_ld(int argc, char* argv[]) {
	// ottieni argomenti
	char* port_str = argv[1];
	int port = atoi(port_str);

	// stampa argomenti
	printf("Port: %d\n", port);

	// apri socket di ascolto
	int ld = socket(AF_INET, SOCK_STREAM, 0);
	if(ld < 0) {
		perror("Creazione socket di ascolto fallita");
		return -1;
	}

	// crea indirizzo socket di ascolto 
	struct sockaddr_in listen_addr;
	memset(&listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(port);
	listen_addr.sin_addr.s_addr = INADDR_ANY;

	// associa indirizzo al socket
	if(bind(ld, (struct sockaddr*) &listen_addr, sizeof(listen_addr)) < 0) {
		perror("Bind socket di ascolto fallita");
		close(ld);
		return -1;
	}
	
	// ascolta
	if(listen(ld, 10) < 0) {
		perror("Impossibile ascoltare su socket di ascolto");
		close(ld);
		return -1;
	}
	
	return ld;
}

int handle_client(int cd) {
	// ottieni indirizzo client
	struct sockaddr_in client_addr;
	int client_addr_len;
	getpeername(cd, (struct sockaddr*) &client_addr, &client_addr_len);

	char addr_str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(client_addr.sin_addr), addr_str, INET_ADDRSTRLEN);

	// ricevi dal client
	char* buf = recv_string(cd);
	if(buf == NULL) {
		printf("Client %s ha attaccato\n", addr_str);
		close(cd);
		return 0;
	}

	printf("Ricevuto %s (%ld byte) da %s, procedo a fare echo...\n", buf, strlen(buf) + 1, addr_str);

	// invia al client
	send_string(cd, buf);
	return 1;
}

int main(int argc, char* argv[]){
	printf("--- Echo server v.0.0 ---\n");

	if(argc < 2) {
		// usa default
		char* nargv[2] = { "", SERV_PORT };
		argv = nargv;
		argc = 2;
	}

	// configura socket d'ascolto	
	int ld = configure_ld(argc, argv);
	if(ld < 0) return 1;

	// configura set master
	fd_set master_set;
	FD_ZERO(&master_set);
	FD_SET(ld, &master_set);
	int fdmax = ld;

	// configura set lettura
	fd_set read_set;
	FD_ZERO(&read_set);

	while(1) {
		// copy master set to read set
		read_set = master_set;

		// select and scan
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
				if(!handle_client(i)) FD_CLR(i, &master_set);
			}

		}
	}
}
