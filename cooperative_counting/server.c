#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"
#include "threads.h"

#define BACKLOG 10     // how many pending connections queue will hold
#define THREAD_COUNT 10
#define WELCOME_MESSAGE_LENGTH 100
#define COUNT_MESSAGE_BUFFER_SIZE 30


int count = 0;
pthread_mutex_t count_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct __arg_t {
    int thread_num;
    int sockfd;
} arg_t;

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

int Create_bound_socket(struct addrinfo *servinfo)
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

void Listen(int sockfd, int backlog)
{
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
}

void Reap_dead_processes(){
    struct sigaction sa;

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
}

int Accept_connection(int sockfd)
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

void *Interact_with_client(void *arg)
{
    arg_t *t_arg = (arg_t *) arg;
    char welcome_message[WELCOME_MESSAGE_LENGTH];
    char count_message_buffer[COUNT_MESSAGE_BUFFER_SIZE];
    char buf[MAXDATASIZE];
    int sockfd, thread_num;

    sockfd = t_arg->sockfd;
    thread_num = t_arg->thread_num;

    sprintf(welcome_message, "Hello, thread %d!", thread_num);

    Send(sockfd, welcome_message, WELCOME_MESSAGE_LENGTH);
    while(1){
        Receive(sockfd, buf, MAXDATASIZE);
        printf("Received Message from thread %d: %s\n", thread_num, buf);
        Pthread_mutex_lock(&count_lock);
        count++;
        Pthread_mutex_unlock(&count_lock);
        sprintf(count_message_buffer, "Current count: %d\n", count);
        Send(sockfd, count_message_buffer, COUNT_MESSAGE_BUFFER_SIZE);
        printf("%s", count_message_buffer);
    }
    return NULL;
}

int main(void)
{
    int i, sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo *servinfo = NULL;
    pthread_t threads[THREAD_COUNT];
    arg_t args[THREAD_COUNT];

    // Setup socket for listening.
    servinfo = get_address_info(NULL, PORT);
    sockfd = Create_bound_socket(servinfo);
    freeaddrinfo(servinfo); // all done with this structure
    Listen(sockfd, BACKLOG);
    Reap_dead_processes();

    printf("server: waiting for connections...\n");

    for (i = 0; i < THREAD_COUNT; i++){
        new_fd = Accept_connection(sockfd);
        if (new_fd == -1)
            continue;
        args[i].thread_num = i;
        args[i].sockfd = new_fd;
        Pthread_create(&threads[i], NULL, Interact_with_client, (void*)&args[i]); 
    }

    return 0;
}
