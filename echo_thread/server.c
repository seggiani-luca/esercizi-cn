#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
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

void* handle_client(void* cdv) {
	int cd = *(int*) cdv;
	free(cdv);

	// ottieni indirizzo client
	struct sockaddr_in client_addr;
	int client_addr_len;
	getpeername(cd,(struct sockaddr*) &client_addr, &client_addr_len);

	char addr_str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(client_addr.sin_addr), addr_str, INET_ADDRSTRLEN);
	
	printf("Connesso a client %s\n", addr_str);
	
	while(1) {
		// ricevi dal client
		char* buf = recv_string(cd);
		if(buf == NULL) {
			printf("Client %s ha attaccato\n", addr_str);
			close(cd);
			break;
		}

		printf("Ricevuto %s (%ld byte) da %s, procedo a fare echo...\n", buf, strlen(buf) + 1, addr_str);

		// invia al client
		send_string(cd, buf);
	}

	return NULL;
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

	while(1) {
		// collega al client
		struct sockaddr_in client_addr;
		int client_len = sizeof(client_addr);

		// alloca in memoria condivisa, per il thread
		int* cd = malloc(sizeof(int));
		*cd = accept(ld, (struct sockaddr*) &client_addr, &client_len);
		if(*cd < 0) {
			perror("Accept fallita");
			free(cd);
			continue;
		}

		// gestisci client via thread
		pthread_t tid;
		pthread_create(&tid, NULL, handle_client, cd);
		pthread_detach(tid);
	}
}
