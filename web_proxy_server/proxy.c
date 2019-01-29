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
#define REQUEST_BUFLEN 1000
#define RESPONSE_BUFLEN 20000

#define HOSTNAME_BUFLEN 100

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

int hostname_is_forbidden(char *hostname, config_t *config){
    site_list_node *node;
    node = config->blocked_sites;

    while (node != NULL){
        if (strcmp(node->site, hostname) == 0)
            return 1;
        node = node->next;
    }
    return 0;
}

void extract_host_name_from_request(char *request, char *hostname){
    sscanf(request, "GET http://%100[^/]%*s HTTP/1.1\r", hostname);
}

int forward_to_remote_host(char *hostname, char *outbound, int outbound_len, char *inbound, int inbound_len){
    int sockfd;
    struct addrinfo *servinfo;

    servinfo = get_address_info(hostname, "80");

    sockfd = create_connected_socket(servinfo);

    freeaddrinfo(servinfo);

    send_on_socket(sockfd, outbound, outbound_len);

    receive_on_socket(sockfd, inbound, inbound_len);

    return 0;
}

int do_proxy_stuff(int sockfd, config_t *config){
    int bytes_received;
    char request_buf[REQUEST_BUFLEN];
    char response_buf[RESPONSE_BUFLEN];
    char hostname[HOSTNAME_BUFLEN];


     bytes_received = receive_on_socket(sockfd, request_buf, REQUEST_BUFLEN);
     if (bytes_received == 0){
         printf("Server received no bytes on socket");
         return -1;
     }
     printf("---------------REQUEST---------------\n%s\n\n", request_buf);

     extract_host_name_from_request(request_buf, hostname);

     if (hostname_is_forbidden(hostname, config)){
         printf("host is forbidden!");
         return 0;
     }


     forward_to_remote_host(hostname, request_buf, REQUEST_BUFLEN, response_buf, RESPONSE_BUFLEN);

     printf("---------------RESPONSE-------------\n%s\n\n", response_buf);
     send_on_socket(sockfd, response_buf, RESPONSE_BUFLEN);

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

        if ((cpid = fork()) == -1){
            perror("fork");
            free_config(&config);
            exit(EXIT_FAILURE);
        }
        else if (cpid == 0){
            close(sockfd);

            rc = do_proxy_stuff(newfd, &config);
            free_config(&config);

            if (rc == -1){
                fprintf(stderr, "Something went wrong on a connection\n");
				exit(EXIT_FAILURE);
            }
            exit(0);
        }
        else
            close(newfd);
    }

    free_config(&config);

    return 0;
}
