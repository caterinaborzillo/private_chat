
// macro for handling errors 
#define handle_error(message)   do { perror(message); exit(EXIT_FAILURE); } while(0)

#define DEBUG           1   // display debug messages
#define SERVER_PORT     6000
#define MAXSIZE         1024
#define REGISTRAZIONE   "registrazione\n"
#define LOGIN           "login\n"
#define FIELDSIZE       20
#define TEXT_SIZE       30

void connessione();
void serialServer(int server_desc);
void registrazione(int socket_desc);
void login(int socket_desc);

/*typedef struct Chat;
typedef struct User;
typedef struct Messaggio;
*/ 

// struttura User
typedef struct User{
    char* nome;
    char* username;
    char* password;
    void* chats;
}User;

typedef struct Messaggio{
    char text[TEXT_SIZE];
    User* src_user;
    User* dest_user;
}Messaggio;

typedef struct Chat{
    Messaggio **messaggi;
    User* u1;
    User* u2;
}Chat;


