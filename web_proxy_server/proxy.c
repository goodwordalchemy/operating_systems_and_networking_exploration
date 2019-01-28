#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "gwa_sockets.h"
#include "gwa_ip_address.h"

#define BACKLOG 10
#define CLIENT_REQUEST_BUFLEN 1000
#define PROXY_REQUEST_BUFLEN 1000
#define CLIENT_RESPONSE_BUFLEN 1000
#define PROXY_RESPONSE_BUFLEN 1000

#define HOSTNAME_LEN 1023

int create_server_socket_listening_on_port(char* port){
    int sockfd;
    struct addrinfo *servinfo;

    servinfo = get_address_info(NULL, port);

    sockfd = create_bound_socket(servinfo);

    freeaddrinfo(servinfo); // all done with this structure

    listen_on_socket(sockfd, BACKLOG);

    reap_dead_processes();

    return sockfd;
}

int do_proxy_stuff(int sockfd){
    int bytes_received;
    char receive_buf[CLIENT_REQUEST_BUFLEN];

     bytes_received = receive_on_socket(sockfd, receive_buf, CLIENT_REQUEST_BUFLEN);
     if (bytes_received == 0){
         printf("Server received no bytes on socket");
         return -1;
     }

     printf("server received the following request:\n----------\n%s\n-------------\n\n", receive_buf);

     return 0;
}

int main(int argc, char *argv[]) {
    int sockfd, newfd, rc;
    pid_t cpid;
    config_t config;
    char *config_filename;

    if (argc != 2){
        fprintf(stderr, "usage: pass the name of your config file.\n");
        return 1;
    }
    config_filename = argv[1];

    init_config(&config);

    parse_config_file(config_filename, &config);

    print_config(&config);
	print_ip_address();

    sockfd = create_server_socket_listening_on_port(config.port);

    while (1){
		printf("waiting for a connection...\n");
        newfd = accept_connection_on_socket(sockfd);
		printf("got a connection...\n");

        if ((cpid = fork()) == -1){
            perror("fork");
            free_config(&config);
            exit(EXIT_FAILURE);
        }
        else if (cpid == 0){
            close(sockfd);

            rc = do_proxy_stuff(newfd);
            if (rc == 0)
                printf("Something went wrong on a connection\n");

            free_config(&config);
            exit(0);
        }
        else
            close(newfd);

    }

    free_config(&config);

    return 0;
}
