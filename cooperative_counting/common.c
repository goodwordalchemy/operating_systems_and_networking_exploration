#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include "common.h"

void *get_internet_address(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

struct addrinfo *get_address_info(char* hostname, char* port)
{
    int rv;
    struct addrinfo hints;
    struct addrinfo *servinfo = NULL;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(hostname, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }
    return servinfo;
}

void Send(int sockfd, char *msg, int msg_length)
{
    if (send(sockfd, msg, msg_length, 0) < 1) {
        perror("send");
        exit(1);
    }
}

void Receive(int sockfd, char *buf, int buf_len)
{
    int numbytes;
    if ((numbytes = recv(sockfd, buf, buf_len-1, 0)) == 0) {
        perror("recv");
        exit(1);
    }

    buf[numbytes] = '\0';
}

