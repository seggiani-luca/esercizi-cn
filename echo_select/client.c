#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "utils.h"

#define SERV_ADDR "127.0.0.1"
#define SERV_PORT "1234"
#define MAX_BUF 100

int config_cd(int argc, char* argv[]) {
	// ottieni argomenti
	char* addr = argv[1];
	char* port_str = argv[2];
	int port = atoi(port_str);

	// stampa argomenti
	printf("Addr: %s\n", addr);
	printf("Port: %d\n", port);

	// apri socket
	int cd = socket(AF_INET, SOCK_STREAM, 0);
	if(cd < 0) {
		perror("Creazione socket fallita");
		return -1;
	}

	// configura socket
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(port);
	inet_pton(AF_INET, addr, &client_addr.sin_addr);

	// connetti al server
	if(connect(cd, (struct sockaddr*) &client_addr, sizeof(client_addr)) < 0) {
		perror("Connessione al server fallita");
		close(cd);
		return -1;
	}

	return cd;
}

void handle_req(int cd) {
	while(1) {
		// invia al server
		char* out_buf = NULL;
		size_t len = 0;

		printf("$ ");
		ssize_t nread = getline(&out_buf, &len, stdin);
		if (nread == -1) {
		    perror("Getline fallita");
		    free(out_buf);
		}

		// rimuovi endline
		out_buf[strcspn(out_buf, "\n")] = '\0';

		if(strcmp(out_buf, "Bye") == 0) {
			printf("Bye!\n");
			break;
		}

		// invia stringa
		send_string(cd, out_buf);
		free(out_buf);
		
		// ricevi dal server
		char* in_buf = recv_string(cd);

		// stampa ed esci
		printf("@ %s\n", in_buf);
		free(in_buf);
	}
}

int main(int argc, char* argv[]){
	if(argc < 3) {
		// usa default
		char* nargv[3] = { "", SERV_ADDR, SERV_PORT };
		argv = nargv;
		argc = 3;
	}

	// apri socket verso server
	int cd = config_cd(argc, argv);
	if(cd < 0) return 1;
	
	// invia richieste al server 
	handle_req(cd);	

	// chiudi socket verso server
	close(cd);

	return 0;
}
