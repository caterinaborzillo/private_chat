#include "main_server.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>  // htons()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <unistd.h>


int sockaddr_len, recv_mess_size, ret;
char buf[MAXSIZE];


void server_routine(int  sock) {
    if (DEBUG) fprintf(stderr, "Inizio server\n");
    // inizializzo client_address a 0 
    struct sockaddr_in client_address = {0};
    
    // loop per gestire incoming connections
    while(1) {
        memset(buf, 0, MAXSIZE);

        sockaddr_len = sizeof(client_address);
        recv_mess_size = recvfrom(sock, buf, MAXSIZE, 0, (struct sockaddr*) &client_address, &sockaddr_len);

        printf("Received: %s\n", buf);
        
        //rinvia la stringa ricevuta al client (user)
        ret = sendto(sock, buf, recv_mess_size, 0, (struct sockaddr *) &client_address, sizeof(client_address));
        if (ret != recv_mess_size) handle_error("Errore, messaggio troppo lungo");
    }
}


int main(int args, char* argv[]) {

    int sock;

    // setto a 0 tutti i campi della struttura dati sockaddr_in:
    // struttura che rappresenta l'endpoint costituito dal server
    struct sockaddr_in server_address = {0};

    //creazione della socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) handle_error("Could not create socket");

    // implementazione struttura dati sockaddr_in relativa al server
    // vogliamo accettare le connessioni da qualsiasi interfaccia:
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);

    // per collegare la socket appena creata all'endpoint server
    ret = bind(sock, (struct sockaddr*) &server_address, sizeof(struct sockaddr_in));
    if (ret < 0) handle_error("Cannot bind address to socket");

    // devo gestire la ricezione dei messaggi 

    server_routine(sock);

    ret = close(sock);
    if (ret) handle_error("Cannot close socket");
}