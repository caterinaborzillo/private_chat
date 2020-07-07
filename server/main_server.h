
// macro for handling errors 
#define handle_error(message)   do { perror(message); exit(EXIT_FAILURE); } while(0)

#define DEBUG           1   // display debug messages
#define SERVER_PORT     6000
#define MAXSIZE         1024
#define REGISTRAZIONE   "registrazione\n"
#define LOGIN           "login\n"
#define FIELDSIZE       20

void connessione();
//void connection_handler(int socket_desc, struct sockaddr_in* client_addr);
void serialServer(int server_desc);
void registrazione(int socket_desc);
void login(int socket_desc);

// struttura User
typedef struct User{
    char* nome;
    char* username;
    char* password;
}User;