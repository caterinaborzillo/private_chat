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

int udp_server;
pthread_t thread;
char curr_username[30];
struct sockaddr_in client_address = {0};
ListAddress_Head addresses;
ListAddress* first;
ListAddress* last;
void inizializzazione_chat(char* curr_username);
int search_n(char* curr_username);
//List_init(ListAddress_Head *addresses);
// variabile globale del socket descriptor del server 
// (che deve passare da protocollo tcp a protocollo udp)
int server_socket;
User** user;
int n;
char primariga[2];
char* dest;
char message[50];
int sockaddr_len, recv_mess_size, ret;
char buf[MAXSIZE];
void connection_handler(int socket_desc, struct sockaddr_in* client_addr);
FILE *f;

int i = 0;

void ricezione_invio_messaggi(struct sockaddr_in client_address, int sock) {
        int ret, z=0;
        while(1) {
        char* p;
        dest=malloc(MAXSIZE);
        ListAddress* l;
        memset(buf, 0, MAXSIZE);
        sockaddr_len = sizeof(client_address);
        recv_mess_size = recvfrom(sock, buf, MAXSIZE, 0, (struct sockaddr*) &client_address, (unsigned int*)&sockaddr_len);
        if (DEBUG) fprintf(stderr, "Messaggio ricevuto: %s\n", buf);
        memset(dest, 0, strlen(dest));
        p = strtok(buf, ":");
        while(p != NULL){
                if (z==0) {
                        char* temp = malloc(strlen(p));
                        memcpy(temp, p, strlen(p));
                        memcpy(dest, temp, strlen(temp));
                        if (DEBUG) fprintf(stderr, "Destinatario: %s\n", dest);
                        if (DEBUG) fprintf(stderr, "Bytes dest: %ld \n", strlen(dest));
                        free(temp);                       
                        //if (DEBUG) fprintf(stderr, "z: %d \n", z);                      
                } else {
                    if(p[ strlen(p) - 1]== '\n'){
                        char* temp = malloc(strlen(p));
                        memcpy(temp, p, strlen(p)-1);
                        memcpy(message, temp, strlen(temp));
                        //if (DEBUG) fprintf(stderr, "z: %d \n", z);                      
                        free(temp);
                        if (DEBUG) fprintf(stderr, "Resto del messaggio: %s \n", message);
                    }
        l =  List_find(&addresses,dest);
        if (DEBUG) fprintf(stderr, "ok, sono dopo Listfind\n");
        if (DEBUG) fprintf(stderr, "struttura trovata: %d, %s\n", l->c_addr.sin_port, l->username_addr);


        if (l != 0 ) {
            if (DEBUG) fprintf(stderr, "ok, destinatario online trovato\n");
            ret = sendto(sock, buf, strlen(buf), 0, (const struct sockaddr*) &(l->c_addr), sizeof(l->c_addr));
            if (ret != strlen(buf)) handle_error("Errore, messaggio troppo lungo");
            if (DEBUG) fprintf(stderr, "dopo la sendto\n");
            if (DEBUG) fprintf(stderr, "porta a cui ho inviato il messaggio: %d \n", (l->c_addr).sin_port);
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
void udp_server_routine(struct sockaddr_in* client_address, int sock) {
    // devo gestire la ricezione dei messaggi 
    if (DEBUG) fprintf(stderr, "Inizio server UDP\n");
    int ret=0;
    
        //printf(buf, "es. paolo: ciao paolo!");
        //printf(buf, "Per specificare il destinatario del messaggio scrivilo come prima parola seguito dai due punti ':' es. paolo: ciao paolo!");
        //ret = sendto(sock, buf, recv_mess_size, 0, (struct sockaddr *) &client_address, sizeof(client_address));
        //if (ret != recv_mess_size) handle_error("Errore, messaggio troppo lungo");

    // to parse the ip address of the client and the port number of the client
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_address->sin_addr), client_ip, INET_ADDRSTRLEN);
    //uint16_t client_port = ntohs(client_address.sin_addr);
    memset(buf, 0, strlen(buf));
    printf(buf, "ciao sono il server COME VA?");
    ListAddress* l = List_find(&addresses,"cat");
    ret = sendto(sock, buf, strlen(buf), 0, (struct sockaddr *) &(l->c_addr), sizeof(l->c_addr));
    if (ret != strlen(buf)) handle_error("Errore, messaggio troppo lungo");
    // loop per gestire incoming connections
    //while(1) {
        //memset(buf, 0, MAXSIZE);
        //sockaddr_len = sizeof(client_address);
        //recv_mess_size = recvfrom(sock, buf, MAXSIZE, 0, (struct sockaddr*) &client_address, (unsigned int*)&sockaddr_len);
        ricezione_invio_messaggi(*(client_address), sock);


        printf("INVIO MESSAGGIO ESEGUITO. %s\n", buf);
        //rinvia la stringa ricevuta dal client (user)
        //ret = sendto(sock, buf, recv_mess_size, 0, (struct sockaddr *) &client_address, sizeof(client_address));
        //if (ret != recv_mess_size) handle_error("Errore, messaggio troppo lungo");
        //printf("Handling client %d\n", client_address.sin_addr.s_addr);

        //memset(buf, 0, strlen(buf)); 
        //printf("Sono in ascolto UDP\n", buf);    if (DEBUG) fprintf(stderr, "Inizio server UDP\n");

        //ret = sendto(sock, buf, recv_mess_size, 0, (struct sockaddr *) &client_address, sizeof(client_address));
        //if (ret != recv_mess_size) handle_error("Errore, messaggio troppo lungo");
        
    //} 
    return;
}

void udp_server_connection(struct sockaddr_in* client_address) {
    // dopo registrazione e login c'è questo thread che si occupa di gestire l'invio e la ricezione dei messaggi
    // voglio iniziare connessione UDP
    int sock;
    if (DEBUG) fprintf(stderr, "INIZIO UDP\n");
    // setto a 0 tutti i campi della struttura dati sockaddr_in:
    // struttura che rappresenta l'endpoint costituito dal server
    //struct sockaddr_in server_address = {0};
    struct sockaddr_in server_udp = {0};

    //creazione della socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) handle_error("Could not create socket");

    server_udp.sin_addr.s_addr = INADDR_ANY; // we want to accept connections from any interface
    server_udp.sin_family      = AF_INET;
    server_udp.sin_port        = htons(SERVER_PORT); // don't forget about network byte order!

    // per collegare la socket appena creata all'endpoint server
    ret = bind(sock, (struct sockaddr*) &server_udp, sizeof(struct sockaddr_in));
    if (ret < 0) handle_error("Cannot bind address to socket");
    i++;
    udp_server=sock;
    udp_server_routine(client_address, sock);
    close(sock);
}

char buf[4000];
size_t buf_len = sizeof(buf);
int msg_len;
int ret;

// chiama serial_server (TCP)
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
    ret = listen(socket_desc, 20);
    if(ret) handle_error("Cannot listen on socket");

    // il SERVER è seriale: gestisce una registrazione/login alla volta
    serialServer(socket_desc);

    return;
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

        if (DEBUG) fprintf(stderr, "Done!\n");
        // Done! vuol dire che è finita la regisrazione/login
        ListAddress* addr_new = (ListAddress*)malloc(sizeof(ListAddress));
        struct sockaddr_in c_addr = {0};
        addr_new->username_addr = malloc(FIELDSIZE);
        //memset(username_addr, 0, FIELDSIZE);
        memcpy((struct sockaddr*)&(addr_new->c_addr),(struct sockaddr*)(&client_addr), sizeof(struct sockaddr_in));
        memcpy((addr_new->username_addr),&curr_username, FIELDSIZE);
        //printf("%s\n", inet_ntoa(client_addr.sin_addr));
        if (DEBUG) fprintf(stderr, "addr_new->username_addr: %s \n", addr_new->username_addr);
        if (DEBUG) fprintf(stderr, "curr_username: %s \n", curr_username);

        ListAddress* res = List_insert(&addresses, addresses.last, addr_new);
        if (DEBUG) fprintf(stderr, "LIST INSERT\n");

        ListAddr_print(&addresses);
        inizializzazione_chat(addr_new->username_addr);
        if (i==0) {
        if (pthread_create(&thread, NULL, (void*)udp_server_connection, (void*)&(addr_new->c_addr)) == -1 ) handle_error("errore nella pthread create");
        if (DEBUG) fprintf(stderr, "A new thread has been created to handle the messages request...\n");

        } else udp_server_routine(&(addr_new->c_addr), udp_server);

        // reset fields in client_addr, perchè ad ogni while accetto connessioni da client diversi
        memset(&client_addr, 0, sizeof(struct sockaddr_in));
    }
    
    if (pthread_detach(thread) == -1) handle_error("errore nella pthread detach");
}
/*
void Addr_new_print(ListAddress* item) {
	printf("addr_new: ", f_item->value);
}
*/

void inizializzazione_chat(char *curr_username){
/*
    // creo la struttura chat per per ogni utente (per ora solo quelli che si sono loggati)
    Chat **chats = (Chat**)malloc(5*sizeof(Chat*)); // per ora max 5 chat
    int j = search_n(curr_username);
    user[j]->chats = chats;
    Messaggio **messaggi = (Messaggio**)malloc(10*sizeof(Messaggio*));
    //es creo una chat
    Chat * c1 = (Chat*)malloc(sizeof(Chat));
    c1->dest = 0;//nome destinatario
    c1->messaggi = messaggi;
    // singolo messaggio
    Messaggio* mess = (Messaggio*)malloc(sizeof(Messaggio));
    mess->text = 0; //buffer che contiene l'intero messaggio
    */
}
int search_n(char* curr_username){
    for(int i=0; i<n;i++) {
        if (user[i]->username==curr_username) return i;
    } return -1;
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
    uint16_t client_port = ntohs(client_addr->sin_port);
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
            // fine registrazione
        }
        // close socket
        // MA prima di chiudere devo passare socket_desc alla variabile globale server_socket
        server_socket = socket_desc;
        ret = close(socket_desc);
        if(ret) handle_error("Cannot close socket for incoming connection");
        return;
    }

void send_tcp_message(char *buf, int socket_desc) {
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
  ListAddress* aux=head->first;
  printf("[");
  while(aux){
    ListAddress* element = (ListAddress*) aux;
    printf("(%d,%s) ", element->c_addr.sin_port, element->username_addr);
    aux=aux->next;
  }
  printf("]\n");
}

// funzione che stampa tutti gli utenti iscritti (per iniziare a messaggiare con uno di loro)
void stampa_utenti(char* curr_username, int socket_desc, char* buf) {
    memset(buf, 0, buf_len);
            sprintf(buf, "Lista di tutti gli utenti: \n");
            send_tcp_message(buf, socket_desc);
   /* for (int j=0; j < n; j++) {
        if (strcmp(user[j]->username, curr_username)!=0) {
            memset(buf, 0, buf_len);
            sprintf(buf, (char*)user[j]->username);
            strcat(buf, "\n");
            send_tcp_message(buf, socket_desc);

        if (DEBUG) fprintf(stderr, "messaggio da inviare al client: %s ...\n",buf);
        }
    }
    */
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
                    n++;
                    // salvo tutti i dati nel file database.txt in modo tale da conservarmi le informazioni degli utenti iscritti
                    f = fopen("database.txt", "a+");
                    if (f == NULL) { handle_error("Impossibile aprire il file"); }
                    // n è il numero di utenti 
                    if (DEBUG) fprintf(stderr, "valore di n: %d \n", n);
                    //fseek(f, 0, SEEK_SET);
                    //sprintf(primariga, "%d", n);answ
                    //int fd = fileno(f);
                    //int nu = (char)n;
                    //fputc(n, f);
                    //if (DEBUG) fprintf(stderr, "primariga: %s \n", primariga);
                    //fputs(primariga, f);
                    //fseek(f, 0, SEEK_END);
                    fprintf(f, "%s,%s,%s\n", u->nome, u->username, u->password);
                    fclose(f);
                    user[n] = u;
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
        int recv_bytes;
            memset(buf, 0, buf_len);
            // username
            sprintf(buf, "Inserisci username: \n");     
            send_tcp_message(buf, socket_desc);

        recv_tcp_message(buf, socket_desc);
        if (DEBUG) fprintf(stderr, "Received username : %s \n",buf);    
        memcpy(username, buf, strlen(buf)-1);
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
        while(buf[recv_bytes-1] != '\n'){
            ret = recv(socket_desc, buf+recv_bytes, 1, 0);
            if (ret == -1) {
                if (errno == EINTR) continue;
                handle_error("errore nella recv"); }
            if (ret == 0) break;
            recv_bytes+= ret;
        } 
        
        if (DEBUG) fprintf(stderr, "Received password: %s \n", buf);
        memcpy(password, buf, strlen(buf)-1);

        if (database_research(username,password)){
        memset(buf, 0, buf_len);
            sprintf(buf, "Login effettuato. Puoi iniziare ora! \n");
            send_tcp_message(buf, socket_desc);
            memcpy(curr_username, username, strlen(username));
            stampa_utenti(curr_username, socket_desc, buf);
            
        // !!!!!!!!!!!!!!!!!!!!
        // funzione che scambia messaggi con gli altri client
        // devo iniziare a usare UDP per scambiare messaggi
        return;
        } else {
            memset(buf, 0, buf_len);
            sprintf(buf, "Credenziali errate, prego rieffettuare il login. \n");
            send_tcp_message(buf, socket_desc);
            recv_tcp_message(buf, socket_desc);
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
    return item;
}
void List_init(ListAddress_Head* head) {
  head->first=0;
  head->last=0;
  head->size=0;
}
int main(int args, char* argv[]) {

    
    User* u;
    char *p;
    char line[200];
    char* res;
    user = malloc(15*sizeof(User));
    memset(line, 0, strlen(line));
    //se il file NON esiste
    if (fileexists("database.txt")==0) {
        n = 0;
        // creo / apro il file in lettura e scrittura 
        f = fopen("database.txt", "a+");
        if (f == NULL) {
            handle_error("Impossibile aprire il file");
        }
        // scrivo sul file che sono presenti 0 utenti iscritti
        fprintf(f, "%d\n", n);
        fclose(f);
    }
    else { // se il file ESISTE
        // apro il file in lettura e scrittura 
        f = fopen("database.txt", "r+");
        if (f == NULL) { handle_error("Impossibile aprire il file"); }
        int i = 0, num = 0;
        while (1){
            int j = 0;
            if (i==0) {
                // prima riga
                res = fgets(line, 200, f);
                if (res == NULL) break;
                n = atoi(line);
                user = (User**)malloc(10*sizeof(User*));
                i++;
            } else { // altre righe con i dati
            res = fgets(line, 200, f);
            if (res == NULL) break;
            p = strtok(line, ",");
            while(p!=NULL) {
                if (j==0) { // nome
                    u = malloc(sizeof(User));
                    u->nome = malloc(FIELDSIZE*sizeof(char));
                    if(p[ strlen(p) - 1]== '\n'){
                        char* temp = malloc(strlen(p));
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
                    u->username = malloc(FIELDSIZE*sizeof(char));
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
                        char* temp = malloc(strlen(p));
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
    
    // come prima cosa il server deve essere in attesa di effettuare il login o la registrazione
    // quindi prima si mette in attesa e poi effettua login e/o registrazione
    List_init(&addresses);
    ListAddr_print(&addresses);
    connessione();
    if (DEBUG) fprintf(stderr, "qui\n");
            

    //ret = close(sock);
    if (ret) handle_error("Cannot close socket");
    free(user);
}

