#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

#include "state.h"
#include "socket_helpers.h"

#define COMPACT_PEER_FORMAT 0

#define DATASIZE_BUFLEN 11 
#define EVENT_BUFLEN 10
#define REQUEST_BUFLEN 255
#define RESPONSE_BUFLEN 8192


int _encode_me(unsigned char l){
    int ans = 0;
      
    ans = ans || isalnum(l);
    ans = ans || l == '~' || l == '.' || l == '_' || l == '-';

    return ans;
}

void url_encode(unsigned char *str, char *buf){
    int i, len, aug;

    len = strlen((char *) str);

    aug = 0;
    for (i = 0; i < len; i++){
        if (_encode_me(str[i])){
            buf[i+aug] = str[i];
        }
        else {
            snprintf(buf+i+aug, 4, "%%%02x", str[i]);
            aug += 2;
        }
    }
    buf[i + aug] = '\0';
}

void _get_request_string(char *buffer){
    char enc_info_hash[3*SHA_DIGEST_LENGTH + 1];
    char left_buf[DATASIZE_BUFLEN];

    url_encode(localstate.info_hash, enc_info_hash);

    if (localstate.is_seeder)
        snprintf(left_buf, 2, "%d", 0);
    else
        snprintf(left_buf, DATASIZE_BUFLEN, "%d", localstate.file_size);

    snprintf(buffer, REQUEST_BUFLEN, 
            "GET /announce?info_hash=%s&"
            "peer_id=%s&port=%s&uploaded=0&downloaded=0&"
            "left=%s&compact=%d&event=started HTTP/1.1\r\n\r\n",
            enc_info_hash, localstate.peer_id, localstate.client_port,
            left_buf, COMPACT_PEER_FORMAT);
}

void _parse_tracker_response(const char *response){
    
}


void _get_response_from_tracker(const char *request, char *response_buf){
    int sockfd;
    struct addrinfo *servinfo;

    char *announce_host = localstate.announce_hostname;
    char *announce_port = localstate.announce_port;

    servinfo  = get_address_info(announce_host, announce_port);
    sockfd = create_connected_socket(servinfo);
    freeaddrinfo(servinfo);

    if (send_on_socket(sockfd, request, strlen(request)) == 0)
        fprintf(stderr, "Error: Could not send announce message to tracker.\n");

    if (receive_on_socket(sockfd, response_buf, RESPONSE_BUFLEN) == 0)
        fprintf(stderr, "Error: Did not receive a announce response from tracker.\n");
}

void print_announce(){
    char announce_request[REQUEST_BUFLEN];
    char announce_response[RESPONSE_BUFLEN];
    
    _get_request_string(announce_request);

    _get_response_from_tracker(announce_request, announce_response);

    printf("\n\nannounce request: %s\n\n", announce_request);
    printf("announce response: %s\n", announce_response);
}
