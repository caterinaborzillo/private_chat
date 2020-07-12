#include <netinet/in.h> // struct sockaddr_in

// macro for handling errors 
#define handle_error(message)   do { perror(message); exit(EXIT_FAILURE); } while(0)

#define DEBUG           1   // display debug messages
#define SERVER_PORT     6000
#define MAXSIZE         4000
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
    void** chats;
} User;

typedef struct Messaggio{
    char *text;
} Messaggio;

typedef struct Chat{
    Messaggio **messaggi;
    User* dest;
} Chat;

typedef struct ListAddress {
    struct ListAddress* prev;
    struct ListAddress* next;
    struct sockaddr_in c_addr;
    char* username_addr;
} ListAddress;

typedef struct ListAddress_Head {
    ListAddress* first;
    ListAddress* last;
    int size; 
} ListAddress_Head;

void List_init(ListAddress_Head* head);
ListAddress* List_find(ListAddress_Head* head, char* username);
ListAddress* List_insert(ListAddress_Head* head, ListAddress* prevoius, ListAddress* addr);
ListAddress* List_detach(ListAddress_Head* head, ListAddress* addr);
 void ListAddr_print(ListAddress_Head* head);

