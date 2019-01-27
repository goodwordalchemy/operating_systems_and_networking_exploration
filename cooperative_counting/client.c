#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include "gwa_sockets.h"

#define KB_BUFFER_SIZE 64

void interact_with_server(int sockfd)
{
    char kb_input[KB_BUFFER_SIZE];
    char buf[MAXDATASIZE];
    receive_on_socket(sockfd, buf, MAXDATASIZE);
    printf("client: received %s\n",buf);
    while(1){
        receive_on_socket(sockfd, buf, MAXDATASIZE);
        printf("client: received %s\n",buf);
        printf("Press enter when you want to increment the counter\n");
        if (fgets(kb_input, sizeof(kb_input), stdin)){
            send_on_socket(sockfd, kb_input, KB_BUFFER_SIZE);
        }
    }

}

// get sockaddr, IPv4 or IPv6:
int main(int argc, char *argv[])
{
    int sockfd;  
    char *client_hostname;
    struct addrinfo *servinfo;

    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }

    client_hostname = argv[1];

    servinfo = get_address_info(client_hostname, PORT);

    sockfd = create_connected_socket(servinfo);

    freeaddrinfo(servinfo); // all done with this structure

    interact_with_server(sockfd);

    close(sockfd);

    return 0;
}
