#include "client.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>  // htons()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <unistd.h>
#include <termios.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/select.h>

#define MAXPW 32
int sock;
int socket1;
int socket2;
char message[MAXSIZE];
char buf[MAXSIZE];
int message_length;
int ret;
struct sockaddr_in from_addr = {0};
unsigned int from_size;
char* quit_command = SERVER_COMMAND;
int quit_command_len = strlen(SERVER_COMMAND);
int k=0;
struct sockaddr_in server_addr_udp = {0};
int i=0;
char buf2[FIELDSIZE];
int n = 0;
ssize_t nchr = 0;
char pw[MAXPW] = {0};
char *p = pw;
pthread_mutex_t lock;


typedef struct handler_args_s
{
     struct sockaddr_in server_addr_udp;
     int sock;
} handler_args_t;

ssize_t getpasswd (char **pw, size_t sz, int mask, FILE *fp);

void invio_ricezione_messaggi(struct sockaddr_in server_addr_udp, int sock);
void invio(void* arg);
void ricezione(void* arg);


void invio_ricezione_messaggi(struct sockaddr_in server_addr_udp, int sock){
    fd_set original_socket;
    fd_set original_stdin;
    fd_set readfds;
    struct timeval tv;
    int numfd;

    // clear the set ahead of time
    FD_ZERO(&original_socket);
    FD_ZERO(&original_stdin);
    FD_ZERO(&readfds);
    // add our descriptors to the set (0 - stands for STDIN)
    FD_SET(sock, &original_socket);//instead of 0 put socket_fd
    FD_SET(sock, &readfds);
    FD_SET(0,&original_stdin);
    numfd=sock+1;

    // wait until either socket has data ready to be recv()d (timeout 10.5 secs)
    tv.tv_sec = 1;
    tv.tv_usec = 1000000;

    if (DEBUG) fprintf(stderr, "Pronto thread disponibile a mandare messaggi\n");
    if (DEBUG) fprintf(stderr, "Pronto thread disponibile a ricevere messaggi\n");
    memset(message, 0, MAXSIZE);
    fgets(message, MAXSIZE, stdin);
    if (DEBUG) fprintf(stderr, "porta in send %d\n", server_addr_udp.sin_port);
    if ((message_length = strlen(message)) > MAXSIZE) handle_error("messaggio troppo lungo");

    // invio messaggi al server
    ret = sendto(sock, message, message_length, 0, ( struct sockaddr*) &server_addr_udp, sizeof(server_addr_udp));
    if (ret != message_length) handle_error("sendto errore messaggio troppo lungo\n");


    while(1) {
    readfds = original_socket;


    int recieve = select(numfd, &readfds, NULL, NULL, &tv);
    if (recieve == -1) perror("select");  // error occurred in select() 
    //else if (recieve == 0)  printf("Timeout occurred! No data after 30.5 seconds.\n"); 
    else {
        // one descritpor have data
        if (FD_ISSET(sock, &readfds)) { // if set to read
            FD_CLR(sock, &readfds); // clear the set 
            // ricezione messaggi
            from_size = sizeof(from_addr);
            memset(buf, 0, MAXSIZE);
            ret = recvfrom(sock, buf, MAXSIZE, 0, ( struct sockaddr*) &from_addr, &from_size);
            if (DEBUG) fprintf(stderr, "Ho ricevuto un messaggio.\n");
            // controllo 
            if (server_addr_udp.sin_addr.s_addr != from_addr.sin_addr.s_addr) handle_error("messaggio ricevuto da una sorgente ignota");
            buf[ret] = '\0';
            printf("Received: %s\n", buf);
        } else {
            
            memset(message, 0, MAXSIZE);
            fgets(message, MAXSIZE, stdin);
            if (DEBUG) fprintf(stderr, "porta in send %d\n", server_addr_udp.sin_port);
            if ((message_length = strlen(message)) > MAXSIZE) handle_error("messaggio troppo lungo");

            // invio messaggi al server
            ret = sendto(sock, message, message_length, 0, ( struct sockaddr*) &server_addr_udp, sizeof(server_addr_udp));
            if (ret != message_length) handle_error("sendto errore messaggio troppo lungo\n");
        }
         
        }
    }
    close(sock);
    return;
}

void invio(void* arg){
    handler_args_t *args = (handler_args_t*)arg;
    if (DEBUG) fprintf(stderr, "Pronto thread disponibile a mandare messaggi\n");
    int i = 0;
    while(1){
        
        if (DEBUG) fprintf(stderr, "porta PRIMA della send (in attesa della fgets) %d\n", server_addr_udp.sin_port);
        memset(message, 0, MAXSIZE);
        fgets(message, MAXSIZE, stdin);
        if ((message_length = strlen(message)) > MAXSIZE) handle_error("messaggio troppo lungo");
        
    // invio messaggi al server
    ret = sendto(args->sock, message, message_length, 0, ( struct sockaddr*) &(args->server_addr_udp), sizeof(args->server_addr_udp));
    if (ret != message_length) handle_error("sendto errore messaggio troppo lungo\n");

    if ((ret == quit_command_len) && !memcmp(buf, quit_command, quit_command_len)) break;

    if (DEBUG) fprintf(stderr, "porta DOPO la send %d\n", args->server_addr_udp.sin_port);
    i++;
    }
    //free(args->server_addr_udp);
    free(args);
    //ret = close(args->sock);
    //if(ret) handle_error("Cannot close socket for incoming connection");
}

void ricezione(void* arg){
    
    handler_args_t *args = (handler_args_t*)arg;

    FILE* f;
    if (DEBUG) fprintf(stderr, "Pronto thread disponibile a ricevere messaggi\n");
    while(1){
        // ricezione messaggi
    from_size = sizeof(from_addr);
    memset(buf, 0, MAXSIZE);

    if (DEBUG) fprintf(stderr, "porta server PRIMA la recvfrom: %d\n", args->server_addr_udp.sin_port);
    if (DEBUG) fprintf(stderr, "indirizzo server PRIMA la recvfrom: %d\n", args->server_addr_udp.sin_addr.s_addr);
    
    ret = recvfrom(args->sock, buf, MAXSIZE, 0, ( struct sockaddr*) &from_addr, &from_size);

    if (DEBUG) fprintf(stderr, "porta server DOPO la dopo recvfrom: %d\n", args->server_addr_udp.sin_port);
    // controllo 
    if (args->server_addr_udp.sin_addr.s_addr != from_addr.sin_addr.s_addr) handle_error("messaggio ricevuto da una sorgente ignota");
    buf[ret] = '\0';
    printf("Received: %s\n", buf);
    
    }
}

void udp_handler(int socket_d) {
        
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock < 0) handle_error("creazione socket client fallita");
        int status = fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);

        if (status == -1){
        perror("calling fcntl");
        }

        pthread_t t1,t2;
        handler_args_t* arg = malloc(sizeof(handler_args_t));
        arg->sock = sock;
        arg->server_addr_udp = server_addr_udp;
        if (pthread_create(&t1, NULL, (void*)ricezione, arg)== -1 ) handle_error("errore nella pthread create");
        if (pthread_create(&t2, NULL, (void*)invio, arg) == -1 ) handle_error("errore nella pthread create");
        i++;
    //invio_ricezione_messaggi(server_addr_udp, socket_client);
        if (pthread_join(t1, NULL) !=0) handle_error("Errore nella join");
        if (pthread_join(t2, NULL) !=0) handle_error("Errore nella join");
    return ;
}

int main(int args, char* argv[]) {

    // registrazione - login 
    connection();

    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init failed\n");
        return 1; }

    int sock;
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) handle_error("creazione socket client fallita");
    int status = fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
    if (status == -1){ perror("calling fcntl"); }
    fd_set original_socket;
    fd_set readfds;
    int numfd;
    struct timeval tv;
    FD_ZERO(&original_socket);
    FD_ZERO(&readfds);
    numfd=sock+1;
    FD_SET(sock, &original_socket);//instead of 0 put socket_fd
    FD_SET(sock, &readfds);
    // wait until either socket has data ready to be recv()d (timeout 10.5 secs)
    tv.tv_sec = 30;
    tv.tv_usec = 30000000;

    pthread_t t;
    handler_args_t* arg = malloc(sizeof(handler_args_t));
    arg->sock = sock;
    arg->server_addr_udp = server_addr_udp;
    if (pthread_create(&t, NULL, (void*)invio, arg) == -1 ) handle_error("errore nella pthread create");
    if (DEBUG) fprintf(stderr, "Thread started..\n");
    //invio_ricezione_messaggi(server_addr_udp, sock);


    while(1) {
        readfds = original_socket;
        int recieve = select(numfd, &readfds, NULL, NULL, &tv);
        if (recieve == -1) perror("select");  // error occurred in select() 
        //else if (recieve == 0)  printf("Timeout occurred! No data after 30.5 seconds.\n"); 
        else {
            // one descritpor have data
            if (FD_ISSET(sock, &readfds)) { // if set to read
            pthread_mutex_lock(&lock);
            FD_CLR(sock, &readfds); // clear the set 
            // ricezione messaggi
            from_size = sizeof(from_addr);
            memset(buf, 0, MAXSIZE);
            ret = recvfrom(sock, buf, MAXSIZE, 0, ( struct sockaddr*) &from_addr, &from_size);
            if (DEBUG) fprintf(stderr, "Ho ricevuto un messaggio.\n");
            // controllo 
            if (server_addr_udp.sin_addr.s_addr != from_addr.sin_addr.s_addr) handle_error("messaggio ricevuto da una sorgente ignota");
            buf[ret] = '\0';
            printf("Received: %s\n", buf);
            pthread_mutex_unlock(&lock);
        }
        }
    }
    pthread_join(t, NULL);
    pthread_mutex_destroy(&lock);
    ret = close(sock);
    if (ret) handle_error("Cannot close socket");

    return 0;
}

#define MAXPW 32


void connection() {
    int ret,bytes_sent,recv_bytes;
    char* pass_mess = PASSWORD_MESSAGE;
    size_t pass_mess_len = strlen(pass_mess);


    // variables for handling a socket
    int socket_desc;
    struct sockaddr_in server_addr = {0}; // some fields are required to be filled with 0

    // create a socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_desc < 0) handle_error("Could not create socket");

    // set up parameters for the connection
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(SERVER_PORT); 

    // initiate a connection on the socket
    ret = connect(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
    if(ret) handle_error("Could not create connection");

    if (DEBUG) fprintf(stderr, "Connection established!\n");

    char buf[1024];
    size_t buf_len = sizeof(buf);
    int msg_len;
    memset(buf, 0, buf_len);

    // display welcome message from server
    recv_bytes = 0;
    do {
        ret = recv(socket_desc, buf + recv_bytes, buf_len - recv_bytes, 0);
        if (ret == -1 && errno == EINTR) continue;
        if (ret == -1) handle_error("Cannot read from the socket");
        if (ret == 0) break;
	   recv_bytes += ret;
    } while ( buf[recv_bytes-1] != '\n' );
    printf("%s", buf);
    // MESSAGGIO DI BENVENUTO RICEVUTO, IL SERVER MI CHIEDE DI DIGITARE 'LOGIN' O 'REGISTRAZIONE'
    while (1) {
        
        // per gestire la visibilità della password
        if (!memcmp(buf, pass_mess, pass_mess_len)){
            memset(buf, 0, buf_len);
            nchr = getpasswd (&p, MAXPW, '*', stdin);
            printf ("\n you entered   : %s  (%zu chars)\n", p, nchr);
            memcpy(buf, p, strlen(p)); // copio la password in buf
            //if (DEBUG) fprintf(stderr, "password p in buf da inviare al server: %s", buf);
            buf[strlen(buf)] = '\n'; // remove '\n' from the end of the message
            msg_len = strlen(buf);
            // send message to server
            bytes_sent=0;
            while ( bytes_sent < msg_len) {
            ret = send(socket_desc, buf + bytes_sent, msg_len - bytes_sent, 0);
            if (ret == -1 && errno == EINTR) continue;
            if (ret == -1) handle_error("Cannot write to the socket");
            bytes_sent += ret;
            }
        }

        // Read a line from stdin
        else {
            if (fgets(buf, sizeof(buf), stdin) != (char*)buf) {
            fprintf(stderr, "Error while reading from stdin, exiting...\n");
            exit(EXIT_FAILURE);
        }

        msg_len = strlen(buf);
        buf[strlen(buf) - 1] = '\n'; // remove '\n' from the end of the message
        // send message to server
        bytes_sent=0;
        while ( bytes_sent < msg_len) {
            ret = send(socket_desc, buf + bytes_sent, msg_len - bytes_sent, 0);
            if (ret == -1 && errno == EINTR) continue;
            if (ret == -1) handle_error("Cannot write to the socket");
            bytes_sent += ret;
        }
        }
        memset(buf, 0, buf_len);
	    // read message from server
	    recv_bytes = 0;
    	do {
            ret = recv(socket_desc, buf + recv_bytes, 1, 0);
            if (ret == -1 && errno == EINTR) continue;
            if (ret == -1) handle_error("Cannot read from the socket");
	        if (ret == 0) break;
	        recv_bytes += ret;
	    } while ( buf[recv_bytes-1] != '\n' );
        // server response
        printf("%s\n", buf); // no need to insert '\0'     

        if (!memcmp(buf, "Login terminated.\n", strlen("Login terminated.\n"))){   
            break;
        }
    }
    // prima di chiudere il socket tcp mi salvo l'indirizzo del server
    memcpy((struct sockaddr*)&(server_addr_udp),(struct sockaddr*)(&server_addr), sizeof(struct sockaddr_in));
    //server_addr_udp = server_addr;
    //if (DEBUG) fprintf(stderr, "Porta server quando passo la struttura: %d\n", server_addr.sin_port);
    if (DEBUG) fprintf(stderr, "Porta server quando passo la struttura: %d\n", server_addr_udp.sin_port);
    /*
    if (i==0) {
        udp_handler(socket_desc);
    }
    else {
        handler_args_t* arg = malloc(sizeof(handler_args_t));
        arg->sock = sock;
        arg->server_addr_udp = server_addr_udp;
        invio(arg);
        ricezione(arg);
    }
    */
        // close the socket
    ret = close(socket_desc);
    if(ret) handle_error("Cannot close socket");

    
    return;

}

/* read a string from fp into pw masking keypress with mask char.
getpasswd will read upto sz - 1 chars into pw, null-terminating
the resulting string. On success, the number of characters in
pw are returned, -1 otherwise.
*/
ssize_t getpasswd (char **pw, size_t sz, int mask, FILE *fp)
{
    if (!pw || !sz || !fp) return -1;       /* validate input   */
#ifdef MAXPW
    if (sz > MAXPW) sz = MAXPW;
#endif

    if (*pw == NULL) {              /* reallocate if no address */
        void *tmp = realloc (*pw, sz * sizeof **pw);
        if (!tmp)
            return -1;
        memset (tmp, 0, sz);    /* initialize memory to 0   */
        *pw =  (char*) tmp;
    }
    size_t idx = 0;         /* index, number of chars in read   */
    int c = 0;

    struct termios old_kbd_mode;    /* orig keyboard settings   */
    struct termios new_kbd_mode;

    if (tcgetattr (0, &old_kbd_mode)) { /* save orig settings   */
        fprintf (stderr, "%s() error: tcgetattr failed.\n", __func__);
        return -1;
    }   /* copy old to new */
    memcpy (&new_kbd_mode, &old_kbd_mode, sizeof(struct termios));

    new_kbd_mode.c_lflag &= ~(ICANON | ECHO);  /* new kbd flags */
    new_kbd_mode.c_cc[VTIME] = 0;
    new_kbd_mode.c_cc[VMIN] = 1;
    if (tcsetattr (0, TCSANOW, &new_kbd_mode)) {
        fprintf (stderr, "%s() error: tcsetattr failed.\n", __func__);
        return -1;
    }

    /* read chars from fp, mask if valid char specified */
    while (((c = fgetc (fp)) != '\n' && c != EOF && idx < sz - 1) ||
            (idx == sz - 1 && c == 127))
    {
        if (c != 127) {
            if (31 < mask && mask < 127)    /* valid ascii char */
                fputc (mask, stdout);
            (*pw)[idx++] = c;
        }
        else if (idx > 0) {         /* handle backspace (del)   */
            if (31 < mask && mask < 127) {
                fputc (0x8, stdout);
                fputc (' ', stdout);
                fputc (0x8, stdout);
            }
            (*pw)[--idx] = 0;
        }
    }
    (*pw)[idx] = 0; /* null-terminate   */

    /* reset original keyboard  */
    if (tcsetattr (0, TCSANOW, &old_kbd_mode)) {
        fprintf (stderr, "%s() error: tcsetattr failed.\n", __func__);
        return -1;
    }

    if (idx == sz - 1 && c != '\n') /* warn if pw truncated */
        fprintf (stderr, " (%s() warning: truncated at %zu chars.)\n",
                __func__, sz - 1);

    return idx; /* number of chars in passwd    */
}

