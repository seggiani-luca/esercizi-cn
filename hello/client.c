#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main(int argc, char* argv[]){
	if(argc < 3) {
		printf("Troppi pochi argomenti\n");
		exit(1);
	}
	
	// ottieni argomenti
	char* addr = argv[1];
	char* port_str = argv[2];
	int port = atoi(port_str);

	// stampa argomenti
	printf("Addr: %s\n", addr);
	printf("Port: %d\n", port);

	// apri socket
	int cd = socket(AF_INET, SOCK_STREAM, 0);

	// configura socket
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(port);
	inet_pton(AF_INET, addr, &client_addr.sin_addr);

	// connetti al server
	if(connect(cd, (struct sockaddr*) &client_addr, sizeof(client_addr)) < 0) {
		perror("Connessione fallita");
		exit(1);
	}

	// ricevi dal server
	char buf[100];
	int ret = recv(cd, buf, sizeof(buf) - 1, 0);

	if(ret == -1) {
		perror("Ricezione fallita");
		exit(1);
	}

	// termina la stringa ricevuta
	buf[ret] = '\0';

	// stampa ed esci
	printf("%s\n", buf);

	return 0;
}
