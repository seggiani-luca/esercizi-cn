#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdint.h>

// struttura messaggio:
// [ dimensione (term. compreso) ]
// [ stringa ]

void send_string(int cd, char* buf) {
	// invia dimensione
	size_t len = strlen(buf) + 1;
	uint32_t n_len = (uint32_t) htonl(len);

	int ret = send(cd, (void*) &n_len, 4, 0);
	if(ret == -1) {
		perror("Invio dimensione fallito");
		exit(1);
	}

	// invia stringa
	ret = send(cd, buf, len, 0);
	if(ret == -1) {
		perror("Invio stringa fallito");
		exit(1);
	}
}

char* recv_string(int cd) {
	uint32_t n_len;

	// ricevi dimensione
	int ret = recv(cd, (void*) &n_len, 4, MSG_WAITALL);
	
	// se è chiuso restituisci NULL
	if(ret == 0) return NULL;
	
	if(ret == -1) {
		perror("Ricezione dimensione fallita");
		exit(1);
	}

	size_t len = (size_t) ntohl(n_len);
	char* buf = (char*) malloc(len);

	// ricevi stringa
	ret = recv(cd, buf, len, MSG_WAITALL);
	if(ret == -1) {
		perror("Ricezione stringa fallita");
		exit(1);
	}
	
	// termina la stringa ricevuta
	buf[len - 1] = '\0';
	
	return buf;
}
