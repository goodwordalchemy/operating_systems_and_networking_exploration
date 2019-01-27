#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include "gwa_sockets.h"
#include "gwa_threads.h"

#define THREAD_COUNT 10
#define WELCOME_MESSAGE_LENGTH 100
#define COUNT_MESSAGE_BUFFER_SIZE 30


int count = 0;
pthread_mutex_t count_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct __arg_t {
    int thread_num;
    int sockfd;
} arg_t;



void *interact_with_client(void *arg)
{
    arg_t *t_arg = (arg_t *) arg;
    char welcome_message[WELCOME_MESSAGE_LENGTH];
    char count_message_buffer[COUNT_MESSAGE_BUFFER_SIZE];
    char buf[MAXDATASIZE];
    int sockfd, thread_num;

    sockfd = t_arg->sockfd;
    thread_num = t_arg->thread_num;

    sprintf(welcome_message, "Hello, thread %d!", thread_num);

    send_on_socket(sockfd, welcome_message, WELCOME_MESSAGE_LENGTH);
    while(1){
        receive_on_socket(sockfd, buf, MAXDATASIZE);
        printf("received essage from thread %d: %s\n", thread_num, buf);
        wrapped_pthread_mutex_lock(&count_lock);
        count++;
        wrapped_pthread_mutex_unlock(&count_lock);
        sprintf(count_message_buffer, "Current count: %d\n", count);
        send_on_socket(sockfd, count_message_buffer, COUNT_MESSAGE_BUFFER_SIZE);
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
    sockfd = create_bound_socket(servinfo);

    freeaddrinfo(servinfo); // all done with this structure
    listen_on_socket(sockfd, BACKLOG);
    reap_dead_processes();

    printf("server: waiting for connections...\n");

    for (i = 0; i < THREAD_COUNT; i++){
        new_fd = accept_connection_on_socket(sockfd);
        if (new_fd == -1)
            continue;
        args[i].thread_num = i;
        args[i].sockfd = new_fd;
        wrapped_pthread_create(&threads[i], NULL, interact_with_client, (void*)&args[i]); 
    }

    return 0;
}
