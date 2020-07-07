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
#define MAXPW 32

int sock;
char message[MAXSIZE];
char buf[MAXSIZE];
int message_length;
int ret;
struct sockaddr_in from_addr;
unsigned int from_size;
char* quit_command = SERVER_COMMAND;
int quit_command_len = strlen(SERVER_COMMAND);

char buf2[FIELDSIZE];
int n = 0;
ssize_t nchr = 0;
char pw[MAXPW] = {0};
char *p = pw;

ssize_t getpasswd (char **pw, size_t sz, int mask, FILE *fp);


int main(int args, char* argv[]) {

    // registrazione - login 
    //printf("Ciao! Benvenuto sul sistema di messaggistica più comoda e più veloce di sempre!\n Prima di iniziare per favore registrati sulla piattaforma con i tuoi dati personali. \n Digita 'login' per effettuare il login con le tue credenziali o 'registrazione' per iscriverti: ");
        connection();
        //registrazione();
        /*
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
    */
    while(1) {
    
    memset(message, 0, MAXSIZE);
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
        //if (DEBUG) fprintf(stderr, "Inviato!!!!!!!!!...\n");
        //if (DEBUG) fprintf(stderr, "Leggendo risposta del server...\n");
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
        
        //if (DEBUG) fprintf(stderr, "Leggendo risposta del server...%s and n of bytes %d\n", buf, ret);

        // server response
        printf("%s\n", buf); // no need to insert '\0'
        //printf("%d\n", ret);
        //printf("%s\n", buf);
        
        
    }
    

    // close the socket
    ret = close(socket_desc);
    if(ret) handle_error("Cannot close socket");

    if (DEBUG) fprintf(stderr, "Exiting...\n");

}

// uso protocollo TCP per la registrazione
void registrazione(){

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

