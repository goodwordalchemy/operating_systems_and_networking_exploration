#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "announce.h"
#include "be_node_utils.h"
#include "socket_helpers.h"
#include "state.h"

#define RECEIVE_BUFLEN 1000
#define PROTOCOL_NAME "URTorrent protocol"
#define HANDSHAKE_BUFLEN (49 + 18 + 1)

void write_handshake_str(char *buf){
    char pstrlen = HANDSHAKE_BUFLEN - 1;
    char *reserved = "00000000";

    snprintf(buf, HANDSHAKE_BUFLEN, "%c%s%s%s%s",
             pstrlen, PROTOCOL_NAME,
             reserved, localstate.info_hash,
             localstate.peer_id);
}

void handle_message_from_peer(i){
    printf("bytes recieved on socket %d\n", i);
}

int _initiate_connection_with_peer(be_node *peer){
    int sockfd;
    struct addrinfo *servinfo;

    char port_str[PORT_BUFLEN];
    char *ip = get_be_node_str(peer, "ip");
    int port = get_be_node_int(peer, "port");

    snprintf(port_str, PORT_BUFLEN, "%d", port);

    servinfo = get_address_info(ip, port_str);
    sockfd = create_connected_socket(servinfo);
    freeaddrinfo(servinfo);

    return sockfd;
}

void initiate_connections_with_peers(fd_set *fds){
    int sockfd;
    be_node **pl;
    char handshake_str[HANDSHAKE_BUFLEN];

    write_handshake_str(handshake_str);
    
    pl = get_peers_list();
    while (*pl){
        sockfd = _initiate_connection_with_peer(*pl);
        if (send_on_socket(sockfd, handshake_str, strlen(handshake_str)) <= 0){
            perror("send");
            close(sockfd);
        }
        else 
            FD_SET(sockfd, fds);
        pl++;
    }
}

int validate_handshake_str(char *handshake_str, char *expected_peer_id){

    return 1;
}

int handle_connection_initiated_by_peer(int listener, fd_set *fds){
    int newfd, nbytes;
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char peer_handshake_str[HANDSHAKE_BUFLEN];
    char my_handshake_str[HANDSHAKE_BUFLEN];

    addrlen = sizeof(remoteaddr);
    newfd = accept(listener, 
                   (struct sockaddr *)&remoteaddr,
                   &addrlen);

    if (newfd == -1){
        perror("accept");
        return -1;
    }

    // validate peer handshake
    if ((nbytes = receive_on_socket(newfd, peer_handshake_str, HANDSHAKE_BUFLEN)) <= 0){
        if (nbytes < 0)
            perror("recv");
        close(newfd);
    }

    if (!validate_handshake_str(peer_handshake_str, NULL)){
        close(newfd);
        return -1;
    }


    write_handshake_str(my_handshake_str);
    if (send_on_socket(newfd, my_handshake_str, strlen(my_handshake_str)) <= 0){
        perror("send");
        close(newfd);
        return -1;
    }
    FD_SET(newfd, fds);

    return newfd;
}

int create_listener_socket(fd_set *fds){
    int listener;
    struct addrinfo *servinfo;
    servinfo = get_address_info(localstate.ip, localstate.client_port);
    listener = create_bound_socket(servinfo);
    freeaddrinfo(servinfo);

    FD_SET(listener, fds);

    return listener;
}

int setup_peer_connections(){
    fd_set master, read_fds;
    int listener, newfd, fdmax, i, nbytes;
    char receive_buf[RECEIVE_BUFLEN];

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    initiate_connections_with_peers(&master);

    listener = create_listener_socket(&master);
    fdmax = listener;

    for (;;){
        read_fds = master;

        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
            perror("select");
            return -1;
        } 

        // loop through sockets looking for data to read
        for (i = 0; i <= fdmax; i++){
            if (FD_ISSET(i, &read_fds)){ // found socket with data to read 
                if (i == listener){
                    newfd = handle_connection_initiated_by_peer(i, &master);
                    if (newfd > fdmax)
                        fdmax = newfd;
                }
                else {
                    if ((nbytes = receive_on_socket(i, receive_buf, RECEIVE_BUFLEN)) <= 0){
                        if (nbytes < 0)
                            perror("recv");
                        close(i);
                        FD_CLR(i, &master);
                    }
                    else {
                        handle_message_from_peer(i);
                    }
                }
            }
        }
    }

    return 0;
}
