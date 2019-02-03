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

#define PSTRLEN 18
#define PROTOCOL_NAME "URTorrent protocol"
#define HANDSHAKE_BUFLEN (49 + PSTRLEN + 1)
#define RESERVED_SECTION "00000000"

void write_handshake_str(char *buf){
    snprintf(buf, HANDSHAKE_BUFLEN, "%c%s%s%s%s",
             PSTRLEN, PROTOCOL_NAME,
             RESERVED_SECTION,
             (char*)localstate.info_hash,
             localstate.peer_id);
}

int validate_handshake_str(int sockfd, char *expected_peer_id){
    int nbytes;
    char peer_handshake_str[HANDSHAKE_BUFLEN];

    char pstrlen;
    char pstr[PSTRLEN + 1];
    char reserved[9];
    char info_hash[SHA_DIGEST_LENGTH+1];
    char peer_id[SHA_DIGEST_LENGTH+1];
    
    if ((nbytes = receive_on_socket(sockfd, peer_handshake_str, HANDSHAKE_BUFLEN)) <= 0){
        printf("DEBUG: nbytes received: %d\n", nbytes);
        if (nbytes < 0)
            perror("recv");
        return 0;
    }

    sscanf(peer_handshake_str, "%c%18c%8s%20c%20c",
           &pstrlen, pstr, reserved,
           info_hash, peer_id);

    pstr[PSTRLEN] = 0;
    info_hash[SHA_DIGEST_LENGTH] = 0;
    peer_id[SHA_DIGEST_LENGTH] = 0;
    printf("DEBUG: thepstrlen: %d\n", pstrlen);
    printf("DEBUG: peer's validation string: %s\n", peer_handshake_str);
    printf("DEBUG: pstr right after sscanf: %s\n", pstr);
    printf("DEBUG: length of pstr right after sscanf: %lu\n", strlen(pstr));


    if (pstrlen != PSTRLEN){
        puts("DEBUG: pstrlen is wrong");
        return 0;
    }

    if (strcmp(pstr, PROTOCOL_NAME) != 0){
        printf("DEBUG: protocol name was wrong: %s\n", pstr);
        return  0;
    }

    if (strcmp(reserved, RESERVED_SECTION) != 0){
        puts("DEBUG: reserved section");
        return 0;
    }

    if (strcmp(info_hash, (char*) localstate.info_hash) != 0){
        printf("info hash length: %lu\n", strlen(info_hash));
        puts("DEBUG: info hash");
        return 0;
    }

    if (expected_peer_id != NULL && strcmp(peer_id, expected_peer_id) != 0){
        puts("DEBUG: expected peer id");
        return 0;
    }

    puts("hooray! validated a peer.\n");

    return 1;
}

void handle_message_from_peer(i){
    printf("bytes recieved on socket %d\n", i);
}

int send_handshake_str(int sockfd){
    int nbytes;
    char handshake_str[HANDSHAKE_BUFLEN];

    write_handshake_str(handshake_str);
    if ((nbytes = send_on_socket(sockfd, handshake_str, strlen(handshake_str))) < 0)
        perror("send");

    return nbytes;

}

int _initiate_connection_with_peer(be_node *peer){
    int sockfd;
    struct addrinfo *servinfo;

    char *expected_peer_id;
    char port_str[PORT_BUFLEN];
    char *ip = get_be_node_str(peer, "ip");
    int port = get_be_node_int(peer, "port");

    snprintf(port_str, PORT_BUFLEN, "%d", port);

    servinfo = get_address_info(ip, port_str);
    sockfd = create_connected_socket(servinfo);
    freeaddrinfo(servinfo);

    if (sockfd == -1)
        return -1;

    if (send_handshake_str(sockfd) <= 0){
        fprintf(stderr, "Error sending handshake\n");
        close(sockfd);
        return -1;
    }

    expected_peer_id = get_be_node_str(peer, "peer id");
    if (!validate_handshake_str(sockfd, expected_peer_id)){
        fprintf(stderr, "handshake did not validate.\n");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

void initiate_connections_with_peers(fd_set *fds){
    int sockfd;
    be_node **pl;
    
    pl = get_peers_list();
    while (*pl){
        if (strcmp(get_be_node_str(*pl, "peer id"), localstate.peer_id) == 0){
            pl++;
            continue;
        }

        if ((sockfd = _initiate_connection_with_peer(*pl)) <= 0)
            fprintf(stderr, "Error connecting with peer at %s:%d\n",
                    get_be_node_str(*pl, "ip"),
                    get_be_node_int(*pl, "port"));

        else {
            FD_SET(sockfd, fds);
            // Send bitfield message
        }
        pl++;
    }
}

int handle_connection_initiated_by_peer(int listener, fd_set *fds){
    int newfd;
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char my_handshake_str[HANDSHAKE_BUFLEN];

    addrlen = sizeof(remoteaddr);
    newfd = accept(listener, 
                   (struct sockaddr *)&remoteaddr,
                   &addrlen);

    if (newfd == -1){
        perror("accept");
        return -1;
    }

    if (!validate_handshake_str(newfd, NULL)){
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
    // Send bitfield message

    return newfd;
}

int create_listener_socket(fd_set *fds){
    int listener;
    struct addrinfo *servinfo;
    servinfo = get_address_info(localstate.ip, localstate.client_port);
    listener = create_bound_socket(servinfo);
    freeaddrinfo(servinfo);

    listen_on_socket(listener, BACKLOG);

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
                    puts("DEBUG: peer wants to share with me!");
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
