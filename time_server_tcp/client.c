#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#define SERV_ADDR "127.0.0.1"
#define SERV_PORT 2345

const char* req_mess = "req";

int config_cd() {
	// apri socket
	int cd = socket(AF_INET, SOCK_STREAM, 0);
	if(cd < 0) {
		perror("Creazione socket fallita");
		return -1;
	}

	// configura indirizzo server
	struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(SERV_PORT);
        inet_pton(AF_INET, SERV_ADDR, &serv_addr.sin_addr);

	if(connect(cd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("Connessione al server fallita");
		close(cd);
		return -1;	
	}

	return cd;
}

void deserialize_time(uint32_t* time) {	
	int sec = ntohl((int) time[0]);
	int min = ntohl((int) time[1]);
	int hour = ntohl((int) time[2]);
	int mday = ntohl((int) time[3]);
	int mon = ntohl((int) time[4]);
	int year = ntohl((int) time[5]);

	printf("%04d-%02d-%02dT%02d:%02d:%02d\n", year, mon, mday, hour, min, sec);
}

int get_time(int cd) {
	// invia richiesta
	int ret = send(
		cd,
		(void*) req_mess,
		sizeof(req_mess),
		0
	);
	if(ret < 0) {
		perror("Invio richiesta fallito");
		return 1;
	}

	// ottieni risposta	
	uint32_t time[6];
	
	ret = recv(
		cd,
		(void*) time,
		sizeof(time),
		MSG_WAITALL
	);
	if(ret < 0) {
		perror("Ricezione risposta fallita");
		return 1;
	}
	if(ret == 0) {
		printf("Connessione chiusa lato server\n");
		close(cd);
		return 0;
	}

	// deserializza	
	deserialize_time(time);

	sleep(1);

	return 1;
}

int main() {
	// configura socket
	int cd = config_cd();
	if(cd == -1) return 1;

	// ottieni tempo
	while(get_time(cd));

	close(cd);
	return 0;
}
