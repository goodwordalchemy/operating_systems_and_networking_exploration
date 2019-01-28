#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "gwa_sockets.h"

#define BACKLOG 10

int create_server_socket_listening_on_port(int port){
    int sockfd;
    struct addrinfo *servinfo;

    servinfo = get_address_info(NULL, port);

    sockfd = create_bound_socket(servinfo);

    freeaddrinfo(servinfo); // all done with this structure

    listen_on_socket(sockfd, BACKLOG);

    reap_dead_processes();

    return sockfd;
}

void do_proxy_stuff(){}

int main(int argc, char *argv[]) {
    int sockfd, newfd;
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

    sockfd = create_server_socket_listening_on_port(config.port);

    while (1){
        newfd = accept_connection_on_socket(sockfd);

        if ((cpid = fork()) == -1){
            perror("fork");
            free_config(&config);
            exit(EXIT_FAILURE);
        }
        else if (cpid == 0){
            close(sockfd);

            do_proxy_stuff();
        }
        else
            close(newfd);

    }

    free_config(&config);

    return 0;
}
