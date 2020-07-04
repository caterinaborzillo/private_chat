#include "client.h"
#include "registrazione.c"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>  // htons()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <unistd.h>

int sock;
char message[MAXSIZE];
char buf[MAXSIZE];
int message_length;
int ret;
struct sockaddr_in from_addr;
int from_size;
char* quit_command = SERVER_COMMAND;
int quit_command_len = strlen(SERVER_COMMAND);

char buf2[FIELDSIZE];
int n = 0;
ssize_t nchr = 0;

typedef struct {
    char nome[FIELDSIZE];
    char cognome[FIELDSIZE];
    char username[FIELDSIZE];
    char email[FIELDSIZE];
    char password[FIELDSIZE];
}User;

User user[20];

int main(int args, char* argv[]) {

    // registrazione - login 
    printf("Ciao! Benvenuto sul sistema di messaggistica più comoda e più veloce di sempre!\n Prima di iniziare per favore registrati sulla piattaforma con i tuoi dati personali. \n Digita 'login' per effettuare il login con le tue credenziali o 'registrazione' per iscriverti: ");
    fgets(buf, MAXSIZE, stdin);
    if (strcmp(buf, "login") == 0) {

    }
    else if (strcmp(buf, "registrazione") == 0) {
        registrazione();
        n++;
        printf("Nome: ");
        fgets(user[n].nome, FIELDSIZE, stdin); 

        printf("Cognome: ");
        fgets(user[n].cognome, FIELDSIZE, stdin);

        printf("Username: ");
        fgets(user[n].username, FIELDSIZE, stdin);

        memset(buf2, 0, FIELDSIZE);
        printf("Email: ");
        fgets(user[n].email, FIELDSIZE, stdin);

        memset(buf2, 0, FIELDSIZE);
        printf("Password: ");
        nchr = getpasswd(user[n].password, FIELDSIZE, '*', stdin);
        printf ("\nYou entered: %s  (%zu chars)\n", user[n].password, nchr);
        
        printf("Vuoi confermare? si/no");
        fgets(buf, FIELDSIZE, stdin);
        if (strcmp(buf, "si")==0) {
            printf("Registrazione effettuata con successo! ");
            // funzione login
            printf("\nUser numero %d.\nNome: %s\tCognome: %s\t Email: %s \n Username: %s",n,user[n].nome, user[n].cognome, user[n].email, user[n].username);
        } else if (strcmp(buf, "no")==0) {
            memset(user[n].nome, 0, FIELDSIZE); //???
            memset(user[n].cognome, 0, FIELDSIZE);
            memset(user[n].username, 0, FIELDSIZE);
            memset(user[n].email, 0, FIELDSIZE);
            memset(user[n].password, 0, FIELDSIZE);
            n--;
            printf("Registrazione non avvenuta. Riprova digitando 'registrazione'");
        }
    else printf("Per favore digita 'login' per effettuare il login con le tue credenziali o 'registrazione' per iscriverti: \n ");
    }
    
    while(1) {
    
    memset(message, 0, MAXSIZE);
    printf("Digita il messaggio da inviare al server: ");
    fgets(message, MAXSIZE, stdin);
    if ((message_length = strlen(message)) > MAXSIZE) handle_error("messaggio troppo lungo");

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) handle_error("creazione socket client fallita");

    // costruisco indirizzo del server
    struct sockaddr_in server_address = {0};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    server_address.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);

    // invio messaggio al server
    ret = sendto(sock, message, message_length, 0, (struct sockaddr*) &server_address, sizeof(server_address));
    if (ret != message_length) handle_error("errore messaggio troppo lungo");

    // dopo il quit command non riceveremo nessun comando dal server, quindi possiamo
    // uscire dal loop
    if (message_length == quit_command_len && !memcmp(message, quit_command, quit_command_len)) break;

    // ritorno del messaggio dal server
    from_size = sizeof(from_addr);
    memset(buf, 0, MAXSIZE);

    ret = recvfrom(sock, buf, MAXSIZE, 0, (struct sockaddr*) &from_addr, &from_size);
    // controllo 
    if (server_address.sin_addr.s_addr != from_addr.sin_addr.s_addr) handle_error("messaggio ricevuto da una sorgente ignota");
    buf[ret] = '\0';
    printf("Received: %s\n", buf);
}
    ret = close(sock);
    if (ret) handle_error("Cannot close socket");
    return 0;

}