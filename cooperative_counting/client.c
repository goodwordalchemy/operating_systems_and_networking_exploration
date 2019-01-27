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
#include "common.h"

#define KB_BUFFER_SIZE 64

int create_connected_socket(struct addrinfo *servinfo)
{
    struct addrinfo *p;
    int sockfd;
    char s[INET6_ADDRSTRLEN];
    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        exit(2);
    }

    inet_ntop(p->ai_family, get_internet_address((struct sockaddr *)p->ai_addr),
            s, sizeof s);

    printf("client: connecting to %s\n", s);
    return sockfd;
}

void interact_with_server(int sockfd)
{
    char kb_input[KB_BUFFER_SIZE];
    char buf[MAXDATASIZE];
    Receive(sockfd, buf, MAXDATASIZE);
    printf("client: received %s\n",buf);
    while(1){
        Receive(sockfd, buf, MAXDATASIZE);
        printf("client: received %s\n",buf);
        printf("Press enter when you want to increment the counter\n");
        if (fgets(kb_input, sizeof(kb_input), stdin)){
            Send(sockfd, kb_input, KB_BUFFER_SIZE);
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
