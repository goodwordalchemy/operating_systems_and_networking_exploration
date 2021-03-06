#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "gwa_sockets.h"
#include "gwa_threads.h"

#define WELCOME_MESSAGE_LENGTH 100
#define COUNT_MESSAGE_BUFFER_SIZE 30

#define NUM_WAITING_CONNECTIONS 10
#define THREAD_COUNT 2


int count = 0; // This is the number that the clients modify in their connections

int n_waiting_connections = 0;
int waiting_connections[NUM_WAITING_CONNECTIONS];
int fill_ptr = 0;
int use_ptr = 0;

pthread_mutex_t count_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t connection_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;

void put_connection(int fd){
    waiting_connections[fill_ptr] = fd;
    fill_ptr = (fill_ptr + 1) % NUM_WAITING_CONNECTIONS;
    n_waiting_connections++;
}

int get_connection(){
    int tmp = waiting_connections[use_ptr];
    use_ptr = (use_ptr + 1) % NUM_WAITING_CONNECTIONS;
    n_waiting_connections--;
    return tmp;
}

void interact_with_client(int sockfd, int thread_num){
    int n_bytes;
    char welcome_message[WELCOME_MESSAGE_LENGTH];
    char count_message_buffer[COUNT_MESSAGE_BUFFER_SIZE];
    char buf[MAXDATASIZE];

    sprintf(welcome_message, "Hello, thread %d!", thread_num);
    send_on_socket(sockfd, welcome_message, WELCOME_MESSAGE_LENGTH);

    while(1){
        /* if (!peer_is_connected(sockfd)){ */
        /*     printf("thread %d: peer must have disconnected\n", thread_num); */
        /*     break; */
        /* } */

        n_bytes = receive_on_socket(sockfd, buf, MAXDATASIZE);
        if (n_bytes == 0){
            printf("thread %d: peer must have disconnected\n", thread_num);
            break;
        }

        printf("received message from thread %d: %s\n", thread_num, buf);
        
        wrapped_pthread_mutex_lock(&count_lock);
        count++;
        wrapped_pthread_mutex_unlock(&count_lock);

        sprintf(count_message_buffer, "Current count: %d\n", count);
        n_bytes = send_on_socket(sockfd, count_message_buffer, COUNT_MESSAGE_BUFFER_SIZE);
        printf("%s", count_message_buffer);
    }
}


void *connection_worker(void *arg)
{
    int sockfd, thread_num;

    thread_num = *(int*)arg;
    free(arg);

    while(1){
        printf("thread_num: %d.  About to acquire the connection lock\n", thread_num);
        wrapped_pthread_mutex_lock(&connection_lock);
        printf("thread_num: %d.  About to wait for connections\n", thread_num);
        while (n_waiting_connections == 0)
            wrapped_pthread_cond_wait(&fill, &connection_lock);

        printf("thread_num: %d.  About to get connection\n", thread_num);
        sockfd = get_connection();
        printf("thread_num: %d, sockfd: %d\n", thread_num, sockfd);
        wrapped_pthread_cond_signal(&empty);
        wrapped_pthread_mutex_unlock(&connection_lock);

        interact_with_client(sockfd, thread_num);

        printf("thread_num: %d freed!\n", thread_num);

    }
    return NULL;
}

int main(void)
{
    int i, sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    pthread_t threads[THREAD_COUNT];
    struct addrinfo *servinfo = NULL;


    // Setup socket for listening.
    servinfo = get_address_info(NULL, PORT);
    sockfd = create_bound_socket(servinfo);

    freeaddrinfo(servinfo); // all done with this structure
    listen_on_socket(sockfd, BACKLOG);
    reap_dead_processes();

    printf("server: waiting for connections...\n");

    for (i = 0; i < THREAD_COUNT; i++){
        int *arg;
        if ((arg = malloc(sizeof(*arg))) == NULL){
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        *arg = i;
        wrapped_pthread_create(&threads[i], NULL, connection_worker, (void*)arg); 
    }

    while (1){
        printf("Papa thread about to accept connection\n");
        new_fd = accept_connection_on_socket(sockfd);
        if (new_fd == -1)
            continue;

        printf("Papa thread just accepted new connection.\n");
        wrapped_pthread_mutex_lock(&connection_lock);

        printf("Papa thread just acquired conection lock\n");
        while (n_waiting_connections == NUM_WAITING_CONNECTIONS)
            wrapped_pthread_cond_wait(&empty, &connection_lock);

        printf("Papa thread about to put connection\n");
        put_connection(new_fd);

        wrapped_pthread_cond_signal(&fill);
        wrapped_pthread_mutex_unlock(&connection_lock);
    }

    return 0;
}
