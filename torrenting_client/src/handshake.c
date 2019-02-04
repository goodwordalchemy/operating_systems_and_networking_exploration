#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "announce.h"
#include "be_node_utils.h"
#include "logging_utils.h"
#include "messages.h"
#include "socket_helpers.h"
#include "state.h"

#define RECEIVE_BUFLEN 1000

#define PSTRLEN 18
#define PROTOCOL_NAME "URTorrent protocol"
#define HANDSHAKE_BUFLEN (49 + PSTRLEN + 1)
#define RESERVED_SECTION "00000000"

void print_bitfield_cell(peer_t *p){
    int i, left, cur, n_pieces;

    left = COLUMN_WIDTH;
    n_pieces = localstate.n_pieces;

    for (i = 0; i < n_pieces; i++){
        cur = p->bitfield >> (n_pieces - 1 - i) & 1;
        printf("%d", cur);
    }
    printf("%*s | ", COLUMN_WIDTH - n_pieces, "");

}

void print_peer_bitfields(){
    int i;
    peer_t *p;
    char *headers[] = {"Sockfd", "Status", "Bitfield", "Down/s", "Up/s"};
    int n_headers = 5;

    printf("Peer bitfields:\n");

    printf("\t");
    for (i = 0; i < n_headers; i++){
        print_str_cell(headers[i]);
    }
    printf("\n");

    print_horizontal_line(n_headers * (3 +COLUMN_WIDTH));

    printf("\t");
    for (i = 0; i<MAX_SOCKFD; i++){
        if ((p = localstate.peers[i]) == NULL)
            continue;

        print_int_cell(i);
        print_str_cell("0101");
        print_bitfield_cell(p);
        print_int_cell(0);
        print_int_cell(0);
    }
    printf("\n");

}

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


    if (pstrlen != PSTRLEN){
        return 0;
    }

    if (strcmp(pstr, PROTOCOL_NAME) != 0){
        return  0;
    }

    if (strcmp(reserved, RESERVED_SECTION) != 0){
        return 0;
    }

    if (strcmp(info_hash, (char*) localstate.info_hash) != 0){
        return 0;
    }

    if (expected_peer_id != NULL && strcmp(peer_id, expected_peer_id) != 0){
        return 0;
    }

    printf("Validated Peer: %s\n", peer_id);

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

void add_peer(int sockfd, int bitfield){
    peer_t *p;
    
    p = malloc(sizeof(p));
    p->bitfield = bitfield;

    localstate.peers[sockfd] = p;
};


void remove_peer(int sockfd){
    free(localstate.peers[sockfd]);
    localstate.peers[sockfd] = NULL;
    close(sockfd);
}

int _initiate_connection_with_peer(be_node *peer, fd_set *fds){
    int sockfd, bitfield;
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

    if (send_bitfield_message(sockfd) <= 0){
        fprintf(stderr, "could not send bitfield message to peer\n");
        close(sockfd);
        return -1;
    }

    if ((bitfield = receive_bitfield_message(sockfd)) < 0){
        fprintf(stderr, "could not receive bitfield message from peer\n");
        close(sockfd);
        return -1;
    }

    add_peer(sockfd, bitfield);
    FD_SET(sockfd, fds);

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

        if ((sockfd = _initiate_connection_with_peer(*pl, fds)) <= 0)
            fprintf(stderr, "Error connecting with peer at %s:%d\n",
                    get_be_node_str(*pl, "ip"),
                    get_be_node_int(*pl, "port"));

        pl++;
    }
}

int handle_connection_initiated_by_peer(int listener, fd_set *fds){
    int newfd, bitfield;
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

    if ((bitfield = receive_bitfield_message(newfd)) < 0){
        fprintf(stderr, "could not receive bitfield message from peer\n");
        close(newfd);
        return -1;
    }

    if (send_bitfield_message(newfd) <= 0){
        fprintf(stderr, "could not send bitfield message to peer\n");
        close(newfd);
        return -1;
    }

    add_peer(newfd, bitfield);
    FD_SET(newfd, fds);
    print_peer_bitfields();

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
    print_peer_bitfields();

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
                        remove_peer(i);
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
