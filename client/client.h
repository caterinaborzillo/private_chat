// macro for handling errors 
#define handle_error(message)   do { perror(message); exit(EXIT_FAILURE); } while(0)

#define DEBUG           1   // display debug messages
#define SERVER_PORT     6000
#define SERVER_ADDRESS  "127.0.0.1"
#define MAXSIZE         1024
#define SERVER_COMMAND  "quit\n"
#define FIELDSIZE       30