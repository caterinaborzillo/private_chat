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
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h>

int udp_server;
pthread_t thread;
char curr_username[30];
char* mittente;
struct sockaddr_in client_address = {0};
ListAddress_Head addresses;
ListAddress* first;
ListAddress* last;
int search_n(char* curr_username);
int server_socket;
User** user;
int n;
char primariga[2];
char* dest;
char message[50];
char def_message[50];
int sockaddr_len, recv_mess_size, ret;
char buf[MAXSIZE];
FILE *f;
int w = 0;
struct sockaddr_in from_addr;
int sock_udp;
char buf[4000];
size_t buf_len = sizeof(buf);
int msg_len;
int ret;

void connection_handler(int socket_desc, struct sockaddr_in* client_addr);
void stampa_utenti_udp(ListAddress* curr_user, int socket);
void stampa_utenti(char* curr_user, int socket_desc, char* buf);

void* ricezione_invio_messaggi(void* sock_id) {  
        int* sock_idVal = (int*)sock_id;
        int sock_udp = *sock_idVal;
        for (int i=0; i<n; i++) {
            if (DEBUG) fprintf(stderr, "user[%d]-> nome: %s \n", i, user[i]->nome);
        }   
        int i = 0;
        dest=malloc(MAXSIZE);
        mittente=malloc(FIELDSIZE);
        struct sockaddr_in client_address_recv = {0};
        while(1) {
        int ret, z=0;
        char* p;
        ListAddress* l;
        memset(buf, 0, MAXSIZE);
        memset(message, 0, strlen(message));
        memset(dest, 0, strlen(dest)); 
        sockaddr_len = sizeof(client_address_recv);
        recv_mess_size = recvfrom(sock_udp, buf, MAXSIZE, 0, (struct sockaddr*) &client_address_recv, (unsigned int*)&sockaddr_len);
        if (DEBUG) fprintf(stderr, "Messaggio ricevuto: %s\n", buf);
        if (DEBUG) fprintf(stderr, "%d\n", client_address_recv.sin_port);

        if (List_find_by_addr(&addresses, client_address_recv)){
            memcpy(&from_addr, &(client_address_recv), sizeof(from_addr));
            l = List_find_by_addr(&addresses, client_address_recv);
            memset(mittente, 0, sizeof(mittente));
            memcpy(mittente,l->username_addr, sizeof(mittente));
        }
        if (!strcmp(buf,"utenti online?\n")) {
            l = List_find_by_addr(&addresses, client_address_recv);
            stampa_utenti_udp(l, sock_udp);
            continue;
        }

        if (!strcmp(buf,"quit\n")) {
            ListAddress* l = List_find_by_addr(&addresses, client_address_recv);
            List_detach(&addresses, l);
            ListAddr_print(&addresses);
        }
        else if (List_find_by_addr(&addresses, client_address_recv)==0) {
            l = List_findby_password(&addresses, buf);

            if (l == NULL) fprintf(stderr, "Dopo =================================== \n");
            memcpy(&(l->c_addr),&client_address_recv, sizeof(client_address_recv));  
            if (DEBUG) fprintf(stderr, "Dopo ===================================\n");
            //if (DEBUG) fprintf(stderr, "inserito nella lista di addr: [%s, %d, %s]: \n", l->username_addr, l->c_addr.sin_addr.s_addr, l->user_pass);
            
            ret = sendto(sock_udp, buf, strlen(buf), 0, (const struct sockaddr*) &client_address_recv, sockaddr_len);
            
            if (ret != strlen(buf)) handle_error("Errore, messaggio troppo lungo\n");
            if (DEBUG) fprintf(stderr, "dopo la sendto, inviato primo messaggio\n"); 
            memset(buf, 0, strlen(buf));
            sprintf(buf, "Perfetto, ora puoi iniziare a messaggiare! \n");
            ret = sendto(sock_udp, buf, strlen(buf), 0, (const struct sockaddr*) &client_address_recv, sockaddr_len);
        } else {    
        
        p = strtok(buf, ":");
        while(p != NULL){
                if (z==0) {
                        char* temp = malloc(strlen(p));
                        memcpy(temp, p, strlen(p));
                        memset(dest, 0, strlen(dest));
                        memcpy(dest, temp, strlen(temp));
                        if (DEBUG) fprintf(stderr, "Destinatario: %s\n", dest);
                        if (DEBUG) fprintf(stderr, "Bytes dest: %ld \n", strlen(dest));
                        free(temp);                       
                        //if (DEBUG) fprintf(stderr, "z: %d \n", z);                      
                } else {
                    if(p[ strlen(p) - 1]== '\n'){
                        p[ strlen(p) - 1]= '\0';
                        memcpy(message, p, strlen(p));
                        if (DEBUG) fprintf(stderr, "Resto del messaggio: %s \n", message);
                    }
                    l =  List_find(&addresses,dest);
                    if (DEBUG) fprintf(stderr, "ok, sono dopo Listfind \n");
                    if (DEBUG) fprintf(stderr, "struttura trovata: %d, %s\n", (l->c_addr).sin_port, l->username_addr);
                    if (l==0) {
                        memset(buf, 0, strlen(buf));
                        sprintf(buf, "Destinatario non in lista. Riprova a mandare il messagggio specificando il destinatario corretto. \n");
                        ret = sendto(sock_udp, buf, strlen(buf), 0, (const struct sockaddr*) &client_address_recv, sockaddr_len);
                        memset(buf, 0, strlen(buf));
                        continue;
                    }

                    if (l != 0 ) {
                        if (DEBUG) fprintf(stderr, "ok, destinatario online trovato\n");
                        sockaddr_len = sizeof(l->c_addr);
                        sprintf(def_message, "%s ti scrive: ", mittente);
                        strcat(def_message, message);
                        ret = sendto(sock_udp, def_message, strlen(def_message), 0, (const struct sockaddr*) &(l->c_addr), sockaddr_len);
                        if (DEBUG) fprintf(stderr, "Ret len: %d=====================\n", ret );
                        if (ret != strlen(def_message)) handle_error("Errore, messaggio troppo lungo");
                        else if (ret == -1) handle_error("ret = -1");
                        if (DEBUG) fprintf(stderr, "dopo la sendto\n");
                        if (DEBUG) fprintf(stderr, "indirizzo a cui ho inviato il messaggio: %d \n", (l->c_addr).sin_addr.s_addr);
                        } else {
                            if (DEBUG) fprintf(stderr, "mess non inviato");
                        }
                }
                z++;
                p = strtok (NULL, ":"); 
        }
        }
    }
}


void udp_server_connection() {    
    int i = 0;


    if (DEBUG) fprintf(stderr, "INIZIO UDP\n");
    
    struct sockaddr_in server_addr_udp = {0};

    //creazione della socket
    sock_udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_udp < 0) handle_error("Could not create socket");

    server_addr_udp.sin_addr.s_addr = INADDR_ANY; // we want to accept connections from any interface
    server_addr_udp.sin_family      = AF_INET;
    server_addr_udp.sin_port        = htons(SERVER_PORT); // don't forget about network byte order!
    
    if (DEBUG) fprintf(stderr, "porta su cui sta in ascolto il server: %d\n", server_addr_udp.sin_port);
    // per collegare la socket appena creata all'endpoint server
    ret = bind(sock_udp, (struct sockaddr*) &server_addr_udp, sizeof(struct sockaddr_in));
    if (ret < 0) handle_error("Cannot bind address to socket");
    
    if (pthread_create(&thread, NULL, (void*)ricezione_invio_messaggi, (void*)&(sock_udp)) == -1 ) handle_error("errore nella pthread create");
    if (pthread_detach(thread) == -1) handle_error("errore nella pthread detach");
    
    //close(sock_udp); //?????????????
}

// chiama serial_server (TCP)
void connessione(){
    int i = 0;
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

    //server_udp = server_addr;
    /* We enable SO_REUSEADDR to quickly restart our server after a crash:
     * for more details, read about the TIME_WAIT state in the TCP protocol */
    int reuseaddr_opt = 1;
    ret = setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
    if(ret) handle_error("Cannot set SO_REUSEADDR option");

    // bind address to socket
    ret = bind(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
    if(ret) handle_error("Cannot bind address to socket");

    // start listening
    ret = listen(socket_desc, 10);
    if(ret) handle_error("Cannot listen on socket");

    // il SERVER è seriale: gestisce una registrazione/login alla volta
    serialServer(socket_desc);

    return;
}

void serialServer(int server_desc) {
    int i = 0;
    if (DEBUG) fprintf(stderr, "Inizio serial server\n");
    // we initialize client_addr to zero
    struct sockaddr_in client_addr = {0};

    // loop to handle incoming connections serially
    int sockaddr_len = sizeof(struct sockaddr_in);
    
    while (1) {
        
        if (DEBUG) fprintf(stderr, "I'm accepting new connections...");

        int client_desc = accept(server_desc, (struct sockaddr*) &client_addr, (socklen_t*) &sockaddr_len);
        if(client_desc == -1 && errno == EINTR) continue; // check for interruption by signals
        if(client_desc < 0) handle_error("Cannot open socket for incoming connection");

        if (DEBUG) fprintf(stderr, "Incoming connection accepted...\n");

        /* We pass the socket descriptor and the address information
         * for the incoming connection to the handler. */
        connection_handler(client_desc, &client_addr);

        if (DEBUG) fprintf(stderr, "Done! Registration/login terminated for %s.\n", curr_username);
        
        ListAddress* addr_new = (ListAddress*)malloc(sizeof(ListAddress));
        char* pass;
        
        addr_new->username_addr = malloc(FIELDSIZE);
        addr_new->user_pass = malloc(FIELDSIZE);
        addr_new->user_pass = search_password(curr_username);
        //addr_new->c_addr = malloc(sizeof(struct sockaddr_in));
        //memcpy((addr_new->user_pass), pass, strlen(addr_new->user_pass));
        memcpy((addr_new->username_addr),&curr_username, FIELDSIZE);

        //printf("%s\n", inet_ntoa(client_addr.sin_addr));
        if (DEBUG) fprintf(stderr, "addr_new->password: %s \n", addr_new->user_pass);
        //if (DEBUG) fprintf(stderr, "curr_username: %s \n", curr_username);

        
        ListAddress* res = List_insert(&addresses, addresses.last, addr_new);
        if (DEBUG) fprintf(stderr, "LIST INSERT\n");
        ListAddr_print(&addresses);
       
        //if (pthread_create(&thread, NULL, (void*)udp_server_connection, (void*)&(addresses)) == -1 ) handle_error("errore nella pthread create");
        //if (DEBUG) fprintf(stderr, "A new thread has been created to handle the messages request...\n");
       if( i == 0){
           i++;
           udp_server_connection();
           
       }
        

        // reset fields in client_addr, perchè ad ogni while accetto connessioni da client diversi
        memset(&client_addr, 0, sizeof(struct sockaddr_in));

        
    }
    
    
    
}

char* search_password(char* username) {
     int i = 0;
    char *password;
    for(i=0; i<n; i++) {
        if ((memcmp(user[i]->username,username, strlen(username)))==0) {
            if (DEBUG) fprintf(stderr, "user[i]->password %s\n", user[i]->password);

            return user[i]->password;

        }
    }
    return 0;
}

int search_n(char* curr_username){
     int i = 0;
    for( i=0; i<n;i++) {
        if (user[i]->username==curr_username) return i;
    } return -1;
}
//client descriptor è il socket che il server ha creato per comunicare con quel preciso client che voleva connettersi 
void connection_handler(int client_desc, struct sockaddr_in* client_addr) {
     int i = 0;
// connection handler
    int recv_bytes;
    char* reg_command = REGISTRAZIONE;
    size_t reg_command_len = strlen(reg_command);
    char* log_command = LOGIN;
    size_t log_command_len = strlen(log_command);

    // to parse the ip address of the client and the port number of the client
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr->sin_addr), client_ip, INET_ADDRSTRLEN);
    uint16_t client_port = ntohs(client_addr->sin_port);
    if (DEBUG) fprintf(stderr, "porta del client ntohs(client_addr->sin_port) %d\n", client_port);
    if (DEBUG) fprintf(stderr, "ip del client con ntohs:  %s\n", client_ip);

    sprintf(buf, "Ciao! Benvenuto sul sistema di messaggistica più comoda e più veloce di sempre!\n Prima di iniziare per favore registrati sulla piattaforma con i tuoi dati personali. \n Digita 'login' per effettuare il login con le tue credenziali o 'registrazione' per iscriverti: \n");
    msg_len = strlen(buf);
    // invio del messaggio di benvenuto
    int bytes_sent = 0;
	while ( bytes_sent < msg_len) {
        ret = send(client_desc, buf + bytes_sent, msg_len - bytes_sent, 0);
        if (ret == -1 && errno == EINTR) continue;
        if (ret == -1) handle_error("Cannot write to the socket");
        bytes_sent += ret;
    }

    memset(buf, 0, buf_len);
        recv_bytes = 0;
        int ret;
        while(buf[recv_bytes-1] != '\n') {
            ret = recv(client_desc, buf+recv_bytes, 1, 0);
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
            login(client_desc);
        }
        if (recv_bytes == reg_command_len && (memcmp(buf, reg_command, reg_command_len)==0)){
            // registrazione
            registrazione(client_desc);
            // fine registrazione+login
        }
        
        ret = close(client_desc);
        if(ret) handle_error("Cannot close socket for incoming connection");

        // qui finisce registrazione e login
        return;
    }

void send_tcp_message(char *buf, int socket_desc) {
     int i = 0;
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
}

void recv_tcp_message(char *buf, int socket_desc){
    memset(buf, 0, buf_len);
     int i = 0;
    int recv_bytes = 0;
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
}

void ListAddr_print(ListAddress_Head* head){
     int i = 0;
  ListAddress* aux=head->first;
  printf("[");
  while(aux){
    ListAddress* element = (ListAddress*) aux;
    printf("(%d,%s,%s) ", (element->c_addr).sin_port, element->username_addr, element->user_pass);
    aux=aux->next;
  }
  printf("]\n");
}

// funzione che stampa tutti gli utenti iscritti (per iniziare a messaggiare con uno di loro)
void stampa_utenti_udp(ListAddress* curr_user, int socket) {
    int i = 0;
    int j;
    int sockaddr_len = 0;
    ListAddress* l = (&addresses)->first;
    while(l){
        sockaddr_len = sizeof(curr_user->c_addr);
        if(i == 0 && strcmp(l->username_addr, (const char*)curr_user)!=0){
            memset(buf, 0, buf_len);
            sprintf(buf, "Lista degli utenti online: \n");
            ret = sendto(socket, buf, strlen(buf), 0, (const struct sockaddr*) &(curr_user->c_addr), sockaddr_len);
            if (ret != strlen(buf)) handle_error("Errore, messaggio troppo lungo 1\n");
            i++;
        }
        if (strcmp(l->username_addr, (const char*)curr_user)!=0) {
            ret = sendto(socket, l->username_addr, strlen(l->username_addr), 0, (const struct sockaddr*) &(curr_user->c_addr), sockaddr_len);
            if (ret != strlen(l->username_addr)) handle_error("Errore, messaggio troppo lungo 2\n");
            ret = sendto(socket, "\n", strlen("\n"), 0, (const struct sockaddr*) &(curr_user->c_addr), sockaddr_len);
            if (ret != strlen("\n")) handle_error("Errore, messaggio troppo lungo 3\n");
        }
        l = l->next;
    }
    if(i == 0){
        memset(buf, 0, buf_len);
        sprintf(buf, "Non ci sono utenti attualmente online!\n");
        ret = sendto(socket, buf, strlen(buf), 0, (const struct sockaddr*) &(curr_user->c_addr), sockaddr_len);
        if (ret != strlen(buf)) handle_error("Errore, messaggio troppo lungo 4\n");
    }
   return;
}


void stampa_utenti(char* curr_user, int socket_desc, char* buf) {
    int i = 0;
    int j;
    if (DEBUG) fprintf(stderr, "messaggio da inviare al client: %d ...\n",n);
    ListAddress* l = (&addresses)->first;
    while(l){
        if(i == 0 && strcmp(l->username_addr, curr_user)!=0){
            memset(buf, 0, buf_len);
            sprintf(buf, "Lista degli utenti online: \n");
            send_tcp_message(buf, socket_desc);
            i++;
        }
        if (strcmp(l->username_addr, curr_user)!=0) {
            send_tcp_message(l->username_addr, socket_desc);
            send_tcp_message("\n", socket_desc);
        }
        l = l->next;
    }
    if(i == 0){
        memset(buf, 0, buf_len);
        sprintf(buf, "Non ci sono utenti attualmente online!\n");
        send_tcp_message(buf, socket_desc);
    }
    memset(buf, 0, buf_len);
    sprintf(buf, "Per specificare il destinatario del messaggio scrivilo come prima parola seguito dai due punti ':' es. paolo: ciao paolo!\n");
    send_tcp_message(buf, socket_desc);
    if (DEBUG) fprintf(stderr, "messaggio da inviare al client: %s ...\n",buf);
    
   return;
}

// ritorna 1 se è presente ritona 0 se il nome non è presente (cioè non si era regisrato)
int database_research(char* username, char* password){
    int i; 
    for (i=0; i<n; i++) {
        if (DEBUG) fprintf(stderr, "username : %s \n", username);
        if (DEBUG) fprintf(stderr, "password : %s \n", password);
        if (DEBUG) fprintf(stderr, "user[%d]->username : %s \n", i, user[i]->username);
        if (DEBUG) fprintf(stderr, "user[%d]->password : %s \n", i, user[i]->password);
        if ((!strcmp(user[i]->username,username)) && (!strcmp(user[i]->password,password))) {
            return 1;
        }
    }
    return 0;
}

// chiama login
void registrazione(int socket_desc) {
     int i = 0;
            User* u = malloc(sizeof(User));
            u->nome = malloc(FIELDSIZE);
            u->username = malloc(FIELDSIZE);
            u->password = malloc(FIELDSIZE);

            memset(buf, 0, buf_len);
            // nome e cognome
            sprintf(buf, "Inserisci nome e cognome: \n");
            send_tcp_message(buf, socket_desc);

        if (DEBUG) fprintf(stderr, "messaggio da inviare al client: %s ...\n",buf);

        recv_tcp_message(buf,socket_desc);
        if (DEBUG) fprintf(stderr, "Received name : %s \n", buf);
        memcpy(u->nome, buf, strlen(buf)-1);


        // username 
        memset(buf, 0, buf_len);
        sprintf(buf, "Inserisci username: \n");
        send_tcp_message(buf, socket_desc);

        recv_tcp_message(buf,socket_desc);

        if (DEBUG) fprintf(stderr, "Received username : %s \n", buf);
        memcpy(u->username, buf, strlen(buf)-1);

        memset(buf, 0, buf_len);
        sprintf(buf, "Inserisci password: \n");
        send_tcp_message(buf, socket_desc);

        recv_tcp_message(buf,socket_desc);

        if (DEBUG) fprintf(stderr, "Received password : %s\n", buf);
        memcpy(u->password, buf, strlen(buf)-1);

        if (DEBUG) fprintf(stderr, "struct dello user:%s %s %s\n", u->nome, u->username, u->password);
        // conferma di registrazione
        memset(buf, 0, buf_len);
        sprintf(buf, "Vuoi confermare i tuoi dati? digita 'si' per confermare oppure 'no' per rieffettuare la registrazione.\n");
        send_tcp_message(buf, socket_desc);

        recv_tcp_message(buf,socket_desc);
        if (DEBUG) fprintf(stderr, "Received answer: %s\n", buf);
        
        if (memcmp(buf, "no", strlen("no"))==0){
            free(u->nome);
            free(u->username);
            free(u->password);
            free(u);
            registrazione(socket_desc);
        
        } else if (memcmp(buf, "si", strlen("si"))==0){    
                    
                    // salvo tutti i dati nel file database.txt in modo tale da conservarmi le informazioni degli utenti iscritti
                    f = fopen("database.txt", "a+");
                    if (f == NULL) { handle_error("Impossibile aprire il file"); }
                    // n è il numero di utenti 
                    if (DEBUG) fprintf(stderr, "valore di n: %d \n", n);
                    fprintf(f, "%s,%s,%s\n", u->nome, u->username, u->password);
                    fclose(f);
                    user[n] = u;
                    n++;
                    memset(buf, 0, buf_len);
                    sprintf(buf, "Registrazione effettuata. Ora premi 'invio' ed effettua il login! \n");
                    send_tcp_message(buf, socket_desc);
                    recv_tcp_message(buf, socket_desc);
            login(socket_desc);
        }
        return;
}

// da qui dovrei chiamare una funzione per la connessione ud tra client e server 
// dovrei passare il fd del socket del client
void login(int socket_desc) {
        char* username = malloc(FIELDSIZE);
        char* password = malloc(FIELDSIZE);
        int recv_bytes = 0;
        int k;
        memset(buf, 0, buf_len);
        // username
        sprintf(buf, "Inserisci username: \n");     
        send_tcp_message(buf, socket_desc);
        recv_tcp_message(buf, socket_desc);

        if (DEBUG) fprintf(stderr, "Received username : %s \n",buf); 
        //if(DEBUG)fprintf(stderr, "--------------------USERNAME: %s", buf );
        memset(username, 0, strlen(username));
        memcpy(username, buf, strlen(buf)-1);
        //if(DEBUG)fprintf(stderr, "--------------------USERNAME: %s", username );
        if (DEBUG) fprintf(stderr, "strlen(buf) %ld: \n",strlen(buf)-1);

        // password
        memset(buf, 0, buf_len);
            sprintf(buf, "Inserisci password: \n");
            send_tcp_message(buf, socket_desc);
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
        //memset(buf, 0, buf_len);
        //recv_bytes = 0;
        while(buf[recv_bytes-1] != '\n') {
            ret = recv(socket_desc, buf+recv_bytes, 1, 0);
            if (ret == -1) {
                if (errno == EINTR) continue;
                handle_error("errore nella recv"); }
            if (ret == 0) break;
            recv_bytes+= ret;
        }
        
        if (DEBUG) fprintf(stderr, "Received password: %s \n", buf);
        memset(password, 0, strlen(password));
        memcpy(password, buf, strlen(buf)-1);
        for (int i=0; i<n; i++) {
            if (DEBUG) fprintf(stderr, "user[i]->username %s \nuser[i]->nome %s \nuser[i]->password %s \n", user[i]->username, user[i]->nome, user[i]->password);
        }
        if (database_research(username,password)){
            memset(buf, 0, buf_len);
            sprintf(buf, "Login effettuato. Puoi iniziare ora! \n");
            send_tcp_message(buf, socket_desc);
            memset(curr_username, 0, strlen(curr_username));
            memcpy(curr_username, username, strlen(username));
            stampa_utenti(curr_username, socket_desc, buf);
            
            return;
        } else {
            memset(buf, 0, buf_len);
            sprintf(buf, "Credenziali errate, prego rieffettuare il login. \n");
            send_tcp_message(buf, socket_desc);
            recv_tcp_message(buf, socket_desc);
            free(username);
            free(password);
            login(socket_desc);
        }
        free(username);
        free(password);
}

// Check if a file exist using stat() function
// return 1 if the file exist otherwise return 0
int fileexists(const char* filename){
    struct stat buffer;
    int exist = stat(filename,&buffer);
    if(exist == 0)
        return 1;
    else // -1
        return 0;
}

// addr è la coppia (username, indirizzo)
ListAddress* List_find(ListAddress_Head* head, char* username) {
    ListAddress* l = head->first;
    while(l){     
        if (DEBUG) fprintf(stderr, "l->username_addr: %s, username da trovare: %s\n", l->username_addr, username);
        if (memcmp(l->username_addr,username, strlen(username))==0) {
            return l;
            if (DEBUG) fprintf(stderr, "(list find) %s\n", l->username_addr);
        }
    l = l->next;
    }
    return 0;
}

ListAddress* List_findby_password(ListAddress_Head* head, char* password) {
    ListAddress* l = head->first;
    while(l){     
        if (DEBUG) fprintf(stderr, "list findby_pass: %s\n", l->username_addr);
        if (DEBUG) fprintf(stderr, "Password Passata: %s\n",password);
        if (DEBUG) fprintf(stderr, "Password Vera: %s\n",l->user_pass);
        if (!memcmp(l->user_pass, password, strlen(l->user_pass))) {
            if (DEBUG) fprintf(stderr, "(list findby_pass) %s\n", l->username_addr);
            if (DEBUG) fprintf(stderr, "%c\n", l==NULL);
            return l;
        }
    l = l->next;
    }
    if (DEBUG) fprintf(stderr, "ESCO CON RETURN O=================\n");
    return 0;
}
// addr è la coppia (username, indirizzo)
ListAddress* List_find_by_addr(ListAddress_Head* head, struct sockaddr_in clientaddress) {
    ListAddress* l = head->first;
    while(l){     
        if (memcmp((const void*)&((l->c_addr).sin_port), (const void*)&(clientaddress.sin_port), sizeof(client_address.sin_port)) == 0) {
            return l;
        }
    l = l->next;
    }
    return 0;
}

ListAddress* List_insert(ListAddress_Head* head, ListAddress* prev, ListAddress* item) {
    if (item->next||item->prev) 
        return 0;
    ListAddress* next = prev ? prev->next : head->first;
    if (prev) {
        item->prev = prev;
        prev->next = item;
    }
    if (next) {
        item->next = next;
        next->prev = item;
    }
    if (!prev)
        head->first = item;
    if (!next) 
        head->last = item;
    ++head->size;
    return item;
}

ListAddress* List_detach(ListAddress_Head* head, ListAddress* item) {
    ListAddress* prev=item->prev;
    ListAddress* next = item->next;
    if (prev) {
        prev->next=next;
    }
    if (next) {
        next->prev = prev;
    }
    if (item==head->first)
        head->first = next;

    if (item==head->last)
        head->last=prev;
    head->size--;
    item->next=item->prev=0;
    free(item->username_addr);
    free(item->user_pass);
    free(item->next);
    free(item->prev);
    free(item);
    return item;
}

void List_init(ListAddress_Head* head) {
  head->first=0;
  head->last=0;
  head->size=0;
}

void freeUsers(User** allUsers){
    int i;
    for(i = 0; i < n; i++){
        User* curr = allUsers[i];
        free(curr->nome);
        free(curr->password);
        free(curr->username);
        free(curr);
    }
    
}
void freeClients(ListAddress_Head* clients){
    ListAddress* l = clients->first;
    ListAddress* tmp = NULL;
    while(l){
        free(l->prev);
        tmp = l->next;
        free(l->username_addr);
        free(l->user_pass);
        free(l->next);
        l = tmp;
    }
    
    free(clients->first);
    free(clients->last);
    //free(clients);
}
void signalHandler(int sig){
    free(mittente);
    free(dest);
    freeClients(&addresses);
    freeUsers(user);
    free(user);
    exit(0);
}

int main() {
    signal(SIGINT, signalHandler);
    User* u = NULL;
    char *p = NULL;
    char* res = NULL;
    char line[200];
    memset(line, 0, 200);
    // if file doesn't exist, it is created
    if (fileexists("database.txt")==0) {
        n = 0;
        // create/open the file in write/read mode 
        f = fopen("database.txt", "a+");
        if (f == NULL) handle_error("It's impossible to open the file");
        // useless
        fprintf(f, "%d\n", n);
        fclose(f);
    }
    else { // if file DOES EXIST (so there are already some subscribed users)
        // open file in write/read mode
        f = fopen("database.txt", "r+");
        if (f == NULL) handle_error("Impossibile aprire il file"); 
        int i = 0, num = 0;
        while (1){
            int j = 0;
            if (i==0) {
                // first line (useless)
                user = (User**)malloc(10*sizeof(User*));
                i++;
            } else { // altre righe con i dati
            res = fgets(line, 200, f);
            if (res == NULL) break;
            p = strtok(line, ",");
            while(p!=NULL) {
                if (j==0) { // nome
                    u = malloc(sizeof(User));
                    u->nome = calloc(FIELDSIZE*sizeof(char), 1);
                    if(p[ strlen(p) - 1]== '\n'){
                        char* temp = calloc(strlen(p), 1);
                        memcpy(temp, p, strlen(p)-1);
                        memcpy(u->nome, temp, strlen(temp));
                        free(temp);
                    }
                    else{
                        memcpy(u->nome, p, strlen(p));
                    }
                    if (DEBUG) fprintf(stderr, "u->nome %s\n", u->nome);
                }
                else if (j==1) { // username
                    u->username = calloc(FIELDSIZE, 1);
                    if(p[ strlen(p) - 1]== '\n'){
                        char* temp = malloc(strlen(p));
                        memcpy(temp, p, strlen(p)-1);
                        memcpy(u->username, temp, strlen(temp));
                        free(temp);
                    }
                    else{
                        memcpy(u->username, p, strlen(p));
                    }
                                      
                    if (DEBUG) fprintf(stderr, "u->username %s\n", u->username);
                } 
                else { // password
                    u->password = malloc(FIELDSIZE*sizeof(char));
                    if(p[ strlen(p) - 1]== '\n'){
                        char* temp = calloc(strlen(p), 1);
                        memcpy(temp, p, strlen(p)-1);
                        memcpy(u->password, temp, strlen(temp));
                        free(temp);
                    }
                    else{
                        memcpy(u->password, p, strlen(p));

                    }
                if (DEBUG) fprintf(stderr, "u->password %s\n", u->password);
                    user[num] = u;
                    num++;
                }
                p = strtok(NULL, ",");
                j++;
            }
            }
        }   
        n = num;
    }
     int i = 0;
    for ( i=0; i<n; i++) {
        if (DEBUG) fprintf(stderr, "user[i]->username %s \nuser[i]->nome %s \nuser[i]->password %s \n", user[i]->username, user[i]->nome, user[i]->password);

    }
    // come prima cosa il server deve essere in attesa di effettuare il login o la registrazione
    // quindi prima si mette in attesa e poi effettua login e/o registrazione
    List_init(&addresses);
    ListAddr_print(&addresses);
    connessione();
    if (DEBUG) fprintf(stderr, "qui\n");
            

    //ret = close(sock);
    if (ret) handle_error("Cannot close socket");
    
}

