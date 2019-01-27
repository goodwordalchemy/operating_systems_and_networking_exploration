#define PORT "3490"  // the port users will be connecting to
#define MAXDATASIZE 100 // max number of bytes we can get at once 
#define BACKLOG 10     // how many pending connections queue will hold

void *get_internet_address();

struct addrinfo *get_address_info();

void listen_on_socket();

int accept_connection_on_socket();

void send_on_socket();

void receive_on_socket();

void reap_dead_processes();
