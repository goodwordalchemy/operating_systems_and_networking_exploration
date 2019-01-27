#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "gwa_sockets.h"

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

int create_bound_socket(struct addrinfo *servinfo)
{
    int sockfd;
    struct addrinfo *p;
    int yes=1;
    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        printf("socket ai_family: %d\n", p->ai_family);
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }
    return sockfd;
}

int accept_connection_on_socket(int sockfd)
{
    int new_fd;
    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    char s[INET6_ADDRSTRLEN];
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
        perror("accept");
        return -1;
    }

    inet_ntop(their_addr.ss_family,
        get_internet_address((struct sockaddr *)&their_addr),
        s, sizeof s);
    printf("server: got connection from %s\n", s);
    return new_fd;
}

void listen_on_socket(int sockfd, int backlog)
{
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
}


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

void send_on_socket(int sockfd, char *msg, int msg_length)
{
    if (send(sockfd, msg, msg_length, 0) < 1) {
        perror("send");
        exit(1);
    }
}

int receive_on_socket(int sockfd, char *buf, int buf_len)
{
    int numbytes = 1;
    if ((numbytes = recv(sockfd, buf, buf_len-1, 0)) == 0) {
        perror("recv");
        exit(1);
    }

    buf[numbytes] = '\0';

    return numbytes;
}

void sigchild_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

void reap_dead_processes(){
    struct sigaction sa;

    sa.sa_handler = sigchild_handler; // reap all dead processes
    if (sigemptyset(&sa.sa_mask) == -1){
        perror("sigemptyset");
        exit(1);
    }
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
}
