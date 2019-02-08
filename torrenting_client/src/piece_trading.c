#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "tracker.h"
#include "be_node_utils.h"
#include "logging_utils.h"
#include "messages.h"
#include "socket_helpers.h"
#include "state.h"

#define SELECT_TIMEOUT 10

#define PSTRLEN 18
#define PROTOCOL_NAME "URTorrent protocol"
#define HANDSHAKE_BUFLEN (49 + PSTRLEN + 1)
#define RESERVED_SECTION "00000000"

void print_timestamp(){
	int timestamp_len = 30;
    time_t timer;
    char buffer[timestamp_len];
    struct tm* tm_info;
	time(&timer);
	tm_info = localtime(&timer);

	strftime(buffer, timestamp_len, "%[%Y-%m-%d %H:%M:%S]\n", tm_info);
	puts(buffer);
}

void write_handshake_str(char *buf){
    snprintf(buf, HANDSHAKE_BUFLEN, "%c%s%s%s%s",
             PSTRLEN, PROTOCOL_NAME,
             RESERVED_SECTION,
             (char*)localstate.info_hash,
             localstate.peer_id);
}

char *validate_handshake_and_get_peer_id(int sockfd, char *expected_peer_id){
    int nbytes;
    char peer_handshake_str[HANDSHAKE_BUFLEN];

    char pstrlen;
    char pstr[PSTRLEN + 1];
    char reserved[9];
    char info_hash[SHA_DIGEST_LENGTH+1];
    char *peer_id;
    
    if ((nbytes = receive_on_socket(sockfd, peer_handshake_str, HANDSHAKE_BUFLEN)) <= 0){
        if (nbytes < 0)
            perror("recv");
        fprintf(stderr, "Error: Received nbytes=%d\n", nbytes);	
        return 0;
    }

     
    peer_id = malloc(SHA_DIGEST_LENGTH+1);
    sscanf(peer_handshake_str, "%c%18c%8s%20c%20c",
           &pstrlen, pstr, reserved,
           info_hash, peer_id);

    pstr[PSTRLEN] = 0;
    info_hash[SHA_DIGEST_LENGTH] = 0;
    peer_id[SHA_DIGEST_LENGTH] = 0;


    if (pstrlen != PSTRLEN){
        fprintf(stderr, "Protocol string length should be %d, but it was %d\n", PSTRLEN, pstrlen);
        free(peer_id);
        return NULL;
    }

    if (strcmp(pstr, PROTOCOL_NAME) != 0){
        fprintf(stderr, "Protocol name should be %s, but it was %s\n", PROTOCOL_NAME, pstr);
        free(peer_id);
        return NULL;
    }

    if (strcmp(reserved, RESERVED_SECTION) != 0){
        fprintf(stderr, "reserved section should be all 0s, but it was %s\n", reserved);
        free(peer_id);
        return NULL;
    }

    if (strcmp(info_hash, (char*) localstate.info_hash) != 0){
        fprintf(stderr, "info hash sent by peer is different than the one we're serving.\n");
        free(peer_id);
        return NULL;
    }

    if (expected_peer_id != NULL && strcmp(peer_id, expected_peer_id) != 0){
        fprintf(stderr, "Peer's peer ID was not what the tracker said it was.\n");
        free(peer_id);
        return NULL;
    }

    printf("Validated Peer: %s\n", peer_id);

    return peer_id;
}

int send_handshake_str(int sockfd){
    int nbytes;
    char handshake_str[HANDSHAKE_BUFLEN];

    write_handshake_str(handshake_str);
    if ((nbytes = send_on_socket(sockfd, handshake_str, strlen(handshake_str))) < 0)
        perror("send");

    return nbytes;

}

void add_peer(int sockfd, char *peer_id){
    peer_t *p;
    
    if ((p = (peer_t *)malloc(sizeof(peer_t))) == NULL)
        perror("malloc");

    p->peer_id = peer_id;
    p->cleared_bitfield = 0;
    p->last_contact = 0;
    p->requested_piece = -1;

    localstate.peers[sockfd] = p;

    printf("Added a new peer on socket %d.\n", sockfd);
};

void remove_peer(int sockfd){
	fprintf(stderr, "Removing peer from socket %d\n", sockfd);

    free(localstate.peers[sockfd]->peer_id);
    free(localstate.peers[sockfd]);
    localstate.peers[sockfd] = NULL;
    close(sockfd);
    if (sockfd == localstate.max_sockfd)
        localstate.max_sockfd--;
}

int _initiate_connection_with_peer(be_node *peer, fd_set *fds){
    int sockfd;
    struct addrinfo *servinfo;

    char *expected_peer_id, *actual_peer_id;
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
    if ((actual_peer_id = validate_handshake_and_get_peer_id(sockfd, expected_peer_id)) == NULL){
        fprintf(stderr, "handshake did not validate.\n");
        close(sockfd);
        return -1;
    }

    if (send_bitfield_message(sockfd) <= 0){
        fprintf(stderr, "could not send bitfield message to peer\n");
        free(actual_peer_id);
        close(sockfd);
        return -1;
    }

    add_peer(sockfd, actual_peer_id);
    FD_SET(sockfd, fds);
    fcntl(sockfd, F_SETFL, O_NONBLOCK);  // set to non-blocking

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

int check_if_already_connected_to_peer(char *peer_id){
    peer_t *p;
    int i;
    for (i = 0; i <= localstate.max_sockfd; i++){
        if ((p = localstate.peers[i]) == NULL)
            continue;
        if (strcmp(p->peer_id, peer_id) == 0)
            return 1;
    }
    return 0;
}

int handle_connection_initiated_by_peer(int listener, fd_set *fds){
    int newfd;
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    char *actual_peer_id;
    char my_handshake_str[HANDSHAKE_BUFLEN];

    addrlen = sizeof(remoteaddr);
    newfd = accept(listener, 
                   (struct sockaddr *)&remoteaddr,
                   &addrlen);

    if (newfd == -1){
        perror("accept");
        return -1;
    }

    if ((actual_peer_id = validate_handshake_and_get_peer_id(newfd, NULL)) == NULL){
        close(newfd);
        return -1;
    }

    if (check_if_already_connected_to_peer(actual_peer_id)){
        fprintf(stderr, "Already connected to peer %s\n", actual_peer_id);
        free(actual_peer_id);
        close(newfd);
        return -1;
    }

    write_handshake_str(my_handshake_str);
    if (send_on_socket(newfd, my_handshake_str, strlen(my_handshake_str)) <= 0){
        perror("send");
        free(actual_peer_id);
        close(newfd);
        return -1;
    }

    if (send_bitfield_message(newfd) <= 0){
        fprintf(stderr, "could not send bitfield message to peer\n");
        free(actual_peer_id);
        close(newfd);
        return -1;
    }

    add_peer(newfd, actual_peer_id);
    fcntl(newfd, F_SETFL, O_NONBLOCK);  // set to non-blocking
    FD_SET(newfd, fds);

    return newfd;
}

int create_listener_socket(fd_set *fds){
    int listener;
    struct addrinfo *servinfo;
    servinfo = get_address_info(localstate.ip, localstate.client_port);
    fprintf(stderr, "creaeting listener socket...");
    listener = create_bound_socket(servinfo);
    fprintf(stderr, "%d\n", listener);
    freeaddrinfo(servinfo);

    listen_on_socket(listener, BACKLOG);

    fcntl(listener, F_SETFL, O_NONBLOCK);  // set to non-blocking
    FD_SET(listener, fds);

    return listener;
}

int setup_peer_connections(){
    struct timeval select_timeout;
    fd_set master, read_fds;
    int listener, newfd, i;
    
    reap_dead_processes();

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    initiate_connections_with_peers(&master);

    listener = create_listener_socket(&master);
    localstate.max_sockfd = listener;

    select_timeout.tv_sec = SELECT_TIMEOUT;
    select_timeout.tv_usec = 0;

    for (;;){

        // handle incoming messagesz
        read_fds = master;

        if (select(localstate.max_sockfd+1, &read_fds, NULL, NULL, &select_timeout) == -1){
            perror("select");
            return -1;
        } 

        // loop through sockets looking for data to read
        for (i = 0; i <= localstate.max_sockfd; i++){
            if (FD_ISSET(i, &read_fds)){ // found socket with data to read 
                if (i == listener){
                    puts("DEBUG: peer wants to share with me!");
                    newfd = handle_connection_initiated_by_peer(i, &master);
                    if (newfd > localstate.max_sockfd)
                        localstate.max_sockfd = newfd;
                }
                else {
                    if (receive_peer_message(i) ==  -1){
                        remove_peer(i);
                        FD_CLR(i, &master);
                    }
                }
            }
        }

        send_request_messages();

        print_my_status();
		print_timestamp();
        print_peer_bitfields();
    }

    return 0;
}
