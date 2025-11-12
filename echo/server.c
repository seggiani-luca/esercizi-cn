#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

int main(int argc, char* argv[]){
	if(argc < 2) {
		printf("Troppi pochi argomenti\n");
		exit(1);
	}
	
	// ottieni argomenti
	char* port_str = argv[1];
	int port = atoi(port_str);

	// stampa argomenti
	printf("Port: %d\n", port);

	// apri socket di ascolto
	int ld = socket(AF_INET, SOCK_STREAM, 0);

	// configura socket di ascolto
	struct sockaddr_in listen_addr;
	memset(&listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(port);
	listen_addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(ld, (struct sockaddr*) &listen_addr, sizeof(listen_addr)) < 0) {
		perror("Bind fallita");
		exit(1);
	}
	
	if(listen(ld, 10) < 0) {
		perror("Listen fallita");
		exit(1);
	}

	while(1) {
		// collega al client
		struct sockaddr_in client_addr;
		int client_len = sizeof(client_addr);

		int cd = accept(ld, (struct sockaddr*) &client_addr, &client_len);
		if(cd < 0) {
			perror("Accept fallita");
			continue;
		}

		char addr_str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(client_addr.sin_addr), addr_str, INET_ADDRSTRLEN);
		
		printf("Connesso a client %s\n", addr_str);

		while(1) {
			// ricevi dal client
			char buf[20];
			int ret = recv(cd, buf, sizeof(buf) - 1, 0);
			if(ret < 0) {
				perror("Ricezione fallita");
				break;
			}

			if(ret == 0) {
				printf("Client %s ha attaccato\n", addr_str);
				break; // client ha attaccato
			}

			buf[ret] = '\0';

			printf("Ricevuto %s da %s, procedo a fare echo...\n", buf, addr_str);

			// invia al client
			ret = send(cd, buf, strlen(buf), 0);
			if(ret < 0) {
				perror("Send fallita");
			}
		}

		close(cd);
	}
}
