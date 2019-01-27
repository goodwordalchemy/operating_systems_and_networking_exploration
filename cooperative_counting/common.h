#define PORT "3490"  // the port users will be connecting to
#define MAXDATASIZE 100 // max number of bytes we can get at once 

void *get_internet_address();

struct addrinfo *get_address_info();

void Send();
void Receive();
