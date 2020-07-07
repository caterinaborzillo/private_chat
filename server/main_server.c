#include "main_server.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>  // htons()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <unistd.h>
#include <termios.h>


User** user;
int n;

int sockaddr_len, recv_mess_size, ret;
char buf[MAXSIZE];
void connection_handler(int socket_desc, struct sockaddr_in* client_addr);

void server_routine(int  sock) {
    if (DEBUG) fprintf(stderr, "Inizio server\n");
    // inizializzo client_address a 0 
    struct sockaddr_in client_address = {0};
    
    // loop per gestire incoming connections
    while(1) {
        memset(buf, 0, MAXSIZE);

        sockaddr_len = sizeof(client_address);
        recv_mess_size = recvfrom(sock, buf, MAXSIZE, 0, (struct sockaddr*) &client_address, (unsigned int*)&sockaddr_len);

        printf("Received: %s\n", buf);
        
        //rinvia la stringa ricevuta al client (user)
        ret = sendto(sock, buf, recv_mess_size, 0, (struct sockaddr *) &client_address, sizeof(client_address));
        if (ret != recv_mess_size) handle_error("Errore, messaggio troppo lungo");
    }
}


char buf[1024];
size_t buf_len = sizeof(buf);
int msg_len;
int ret;

void connessione(){
    int socket_desc;

    // some fields are required to be filled with 0
    struct sockaddr_in server_addr = {0};

     // we will reuse it for accept()

    // initialize socket for listening
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if(socket_desc < 0) handle_error("Could not create socket");

    server_addr.sin_addr.s_addr = INADDR_ANY; // we want to accept connections from any interface
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(SERVER_PORT); // don't forget about network byte order!

    /* We enable SO_REUSEADDR to quickly restart our server after a crash:
     * for more details, read about the TIME_WAIT state in the TCP protocol */
    int reuseaddr_opt = 1;
    ret = setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
    if(ret) handle_error("Cannot set SO_REUSEADDR option");

    // bind address to socket
    ret = bind(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
    if(ret) handle_error("Cannot bind address to socket");

    // start listening
    ret = listen(socket_desc, 20);
    if(ret) handle_error("Cannot listen on socket");

    // il SERVER è seriale: gestisce una registrazione/login alla volta
    serialServer(socket_desc);

    
    // loop di registrazione
    //while(1) {
    //if (DEBUG) fprintf(stderr, "Done!\n");
}

void serialServer(int server_desc) {
    if (DEBUG) fprintf(stderr, "Inizio serial server\n");
    // we initialize client_addr to zero
    struct sockaddr_in client_addr = {0};

    // loop to handle incoming connections serially
    int sockaddr_len = sizeof(struct sockaddr_in);
    while (1) {
        int client_desc = accept(server_desc, (struct sockaddr*) &client_addr, (socklen_t*) &sockaddr_len);
        if(client_desc == -1 && errno == EINTR) continue; // check for interruption by signals
        if(client_desc < 0) handle_error("Cannot open socket for incoming connection");

        if (DEBUG) fprintf(stderr, "Incoming connection accepted...\n");

        /* We pass the socket descriptor and the address information
         * for the incoming connection to the handler. */
        connection_handler(client_desc, &client_addr);

        //if (DEBUG) fprintf(stderr, "Done!\n");

        // reset fields in client_addr, perchè ad ogni while accetto connessioni da client diversi
        memset(&client_addr, 0, sizeof(struct sockaddr_in));
    }
}


void connection_handler(int socket_desc, struct sockaddr_in* client_addr) {
// connection handler
    int recv_bytes;
    char* reg_command = REGISTRAZIONE;
    size_t reg_command_len = strlen(reg_command);
    char* log_command = LOGIN;
    size_t log_command_len = strlen(log_command);

    // to parse the ip address of the client and the port number of the client
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr->sin_addr), client_ip, INET_ADDRSTRLEN);
    //uint16_t client_port = ntohs(client_addr->sin_port);
    if (DEBUG) fprintf(stderr, "I'm writing the welcome message..!\n");

    sprintf(buf, "Ciao! Benvenuto sul sistema di messaggistica più comoda e più veloce di sempre!\n Prima di iniziare per favore registrati sulla piattaforma con i tuoi dati personali. \n Digita 'login' per effettuare il login con le tue credenziali o 'registrazione' per iscriverti: \n");
    msg_len = strlen(buf);
    // invio del messaggio di benvenuto
    int bytes_sent = 0;
	while ( bytes_sent < msg_len) {
        ret = send(socket_desc, buf + bytes_sent, msg_len - bytes_sent, 0);
        if (ret == -1 && errno == EINTR) continue;
        if (ret == -1) handle_error("Cannot write to the socket");
        bytes_sent += ret;
    }

    memset(buf, 0, buf_len);
        recv_bytes = 0;
        int ret;
        while(buf[recv_bytes-1] != '\n') {
            ret = recv(socket_desc, buf+recv_bytes, 1, 0);
            if (ret == -1) {
                if (errno == EINTR) continue;
                handle_error("errore nella recv");
            }
            if (ret == 0) break;
            recv_bytes+= ret;
        }
        if (DEBUG) fprintf(stderr, "Received command of %d bytes: %s \n",recv_bytes, buf);

        if (recv_bytes == log_command_len && !memcmp(buf, log_command, log_command_len)) {
            // login
            login(socket_desc);
        }
        if (recv_bytes == reg_command_len && (memcmp(buf, reg_command, reg_command_len)==0)){
            // registrazione
            registrazione(socket_desc);
            
        printf("Registrazione terminata");
        // fine registrazione
        }
        // close socket
        ret = close(socket_desc);
        if(ret) handle_error("Cannot close socket for incoming connection");
        
        

    }


void registrazione(int socket_desc) {
            User* u = malloc(sizeof(User));
            u->nome = malloc(FIELDSIZE*sizeof(char));
            u->username = malloc(FIELDSIZE*sizeof(char));
            u->password = malloc(FIELDSIZE*sizeof(char));
            n++;
            user[n] = u;

            int recv_bytes;
            memset(buf, 0, buf_len);
            // nome e cognome
            sprintf(buf, "Inserisci nome e cognome: \n");
            int bytes_sent=0;
            buf_len = strlen(buf);
            while(bytes_sent < buf_len) {
                ret = send(socket_desc, buf+bytes_sent, 1, 0);
                if (ret == -1) {
                    if (errno == EINTR) continue;
                    handle_error("errore nella send");
                }     
                bytes_sent+= ret;
            }
        if (DEBUG) fprintf(stderr, "messaggio da inviare al client: %s ...\n",buf);

        memset(buf, 0, buf_len);
        recv_bytes = 0;

        int ret;
        while(buf[recv_bytes-1] != '\n') {
            ret = recv(socket_desc, buf+recv_bytes, 1, 0);
            if (ret == -1) {
                if (errno == EINTR) continue;
                handle_error("errore nella recv");
            }
            if (ret == 0) break;
            recv_bytes+= ret;
        }
        if (DEBUG) fprintf(stderr, "Received name of %d bytes: %s \n",recv_bytes, buf);
        memcpy(u->nome, buf, strlen(buf)-1);

        if (DEBUG)fprintf(stderr, "Nome in array: %s \n", user[n]->nome);

        // username 
        memset(buf, 0, buf_len);
            sprintf(buf, "Inserisci username: \n");
            bytes_sent=0;
            buf_len = strlen(buf);
            while(bytes_sent < buf_len) {
                ret = send(socket_desc, buf+bytes_sent, 1, 0);
                if (ret == -1) {
                    if (errno == EINTR) continue;
                    handle_error("errore nella send");
                }
                bytes_sent+= ret;
            }
        memset(buf, 0, buf_len);
        recv_bytes = 0;
        while(buf[recv_bytes-1] != '\n') {
            ret = recv(socket_desc, buf+recv_bytes, 1, 0);
            if (ret == -1) {
                if (errno == EINTR) continue;
                handle_error("errore nella recv");
            }
            if (ret == 0) break;
            recv_bytes+= ret;
        }
        if (DEBUG) fprintf(stderr, "Received username of %d bytes: %s \n",recv_bytes, buf);
        memcpy(u->username, buf, strlen(buf)-1);

        memset(buf, 0, buf_len);
            sprintf(buf, "Inserisci password: \n");
            bytes_sent=0;
            buf_len = strlen(buf);
            while(bytes_sent < buf_len) {
                ret = send(socket_desc, buf+bytes_sent, 1, 0);
                if (ret == -1) {
                    if (errno == EINTR) continue;
                    handle_error("errore nella send");
                }
                bytes_sent+= ret;
            }
        memset(buf, 0, buf_len);
        recv_bytes = 0;
        while(buf[recv_bytes-1] != '\n') {
            ret = recv(socket_desc, buf+recv_bytes, 1, 0);
            if (ret == -1) {
                if (errno == EINTR) continue;
                handle_error("errore nella recv");
            }
            if (ret == 0) break;
            recv_bytes+= ret;
        }
        if (DEBUG) fprintf(stderr, "Received password of %d bytes: %s\n",recv_bytes, buf);
        memcpy(u->password, buf, strlen(buf)-1);

        memset(buf, 0, buf_len);
            sprintf(buf, "Registrazione effettuata. Ora premi 'invio' ed effettua il login! \n");
            bytes_sent=0;
            buf_len = strlen(buf);
            while(bytes_sent < buf_len) {
                ret = send(socket_desc, buf+bytes_sent, 1, 0);
                if (ret == -1) {
                    if (errno == EINTR) continue;
                    handle_error("errore nella send");
                }
                bytes_sent+= ret;
            }
        if (DEBUG) fprintf(stderr, "struct dello user: %s %s %s", u->nome, u->username, u->password);
        // conferma di registrazione
        memset(buf, 0, buf_len);
            sprintf(buf, "Vuoi confermare i tuoi dati? digita 'si' per confermare oppure 'no' per rieffettuare la registrazione.\n");
            bytes_sent=0;
            buf_len = strlen(buf);
            while(bytes_sent < buf_len) {
                ret = send(socket_desc, buf+bytes_sent, 1, 0);
                if (ret == -1) {
                    if (errno == EINTR) continue;
                    handle_error("errore nella send");
                }
                bytes_sent+= ret;
            }
        memset(buf, 0, buf_len);
        recv_bytes = 0;
        while(buf[recv_bytes-1] != '\n') {
            ret = recv(socket_desc, buf+recv_bytes, 1, 0);
            if (ret == -1) {
                if (errno == EINTR) continue;
                handle_error("errore nella recv");
            }
            if (ret == 0) break;
            recv_bytes+= ret;
        }
        if (DEBUG) fprintf(stderr, "Received answer: %s\n", buf);
        
        if (memcmp(buf, "no", strlen("no"))==0){
            registrazione(socket_desc);
        
        } else login(socket_desc);
}

void login(int socket_desc) {
        int recv_bytes;
            memset(buf, 0, buf_len);
            // username
            sprintf(buf, "Inserisci username: \n");
            int bytes_sent=0;
            buf_len = strlen(buf);
            while(bytes_sent < buf_len) {
                ret = send(socket_desc, buf+bytes_sent, 1, 0);
                if (ret == -1) {
                    if (errno == EINTR) continue;
                    handle_error("errore nella send");
                }     
                bytes_sent+= ret;
            }

        memset(buf, 0, buf_len);
        recv_bytes = 0;

        int ret;
        while(buf[recv_bytes-1] != '\n') {
            ret = recv(socket_desc, buf+recv_bytes, 1, 0);
            if (ret == -1) {
                if (errno == EINTR) continue;
                handle_error("errore nella recv");
            }
            if (ret == 0) break;
            recv_bytes+= ret;
        }
        if (DEBUG) fprintf(stderr, "Received username of %d bytes: %s \n",recv_bytes, buf);

        // password
        memset(buf, 0, buf_len);
            sprintf(buf, "Inserisci password: \n");
            bytes_sent=0;
            buf_len = strlen(buf);
            while(bytes_sent < buf_len) {
                ret = send(socket_desc, buf+bytes_sent, 1, 0);
                if (ret == -1) {
                    if (errno == EINTR) continue;
                    handle_error("errore nella send");
                }
                bytes_sent+= ret;
            }
        memset(buf, 0, buf_len);
        recv_bytes = 0;
        while(buf[recv_bytes-1] != '\n') {
            ret = recv(socket_desc, buf+recv_bytes, 1, 0);
            if (ret == -1) {
                if (errno == EINTR) continue;
                handle_error("errore nella recv");
            }
            if (ret == 0) break;
            recv_bytes+= ret;
        }
        while(buf[recv_bytes-1] != '\n') {
            ret = recv(socket_desc, buf+recv_bytes, 1, 0);
            if (ret == -1) {
                if (errno == EINTR) continue;
                handle_error("errore nella recv");
            }
            if (ret == 0) break;
            recv_bytes+= ret;
        }
        if (DEBUG) fprintf(stderr, "Received password of %d bytes: %s \n",recv_bytes, buf);

        memset(buf, 0, buf_len);
            sprintf(buf, "Login effettuata. Puoi iniziare ora! \n");
            bytes_sent=0;
            buf_len = strlen(buf);
            while(bytes_sent < buf_len) {
                ret = send(socket_desc, buf+bytes_sent, 1, 0);
                if (ret == -1) {
                    if (errno == EINTR) continue;
                    handle_error("errore nella send");
                }
                bytes_sent+= ret;
            }

        // funzione che scambia messaggi con gli altri client
}



int main(int args, char* argv[]) {
    n = -1;
    user = malloc(15*sizeof(User));
    // come prima cosa il server deve essere in attesa di effettuare il login o la registrazione
    // quindi prima si mette in attesa e poi effettua login e/o registrazione
    connessione();

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
    free(user);
}

