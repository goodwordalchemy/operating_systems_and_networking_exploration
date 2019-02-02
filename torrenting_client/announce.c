#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

#include "http_response_parse.h"
#include "state.h"
#include "socket_helpers.h"

#define COMPACT_PEER_FORMAT 0

#define DATASIZE_BUFLEN 11 
#define EVENT_BUFLEN 10
#define REQUEST_BUFLEN 255
#define RESPONSE_BUFLEN 8192

#define COLUMN_WIDTH 15


char *announce_keys[] = {"complete", "downloaded", "incomplete", "interval", "min interval"};
int n_announce_keys = 5;

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


/*
This function does a lot of things worth noting:
* makes an announment to the tracker.
* stores the response from the tracker on the heap.
* returns the response from the tracker.
* parses the response from the tracker and stores it on the heap in the global variable: tracker_response
*/
http_response_t *announce(){
    char announce_request[REQUEST_BUFLEN];
    char announce_response[RESPONSE_BUFLEN];
    
    _get_request_string(announce_request);

    _get_response_from_tracker(announce_request, announce_response);

    /* printf("\n\nannounce request: %s\n\n", announce_request); */
    /* printf("announce response: %s\n", announce_response); */

    http_response_t *r = extract_response_content(announce_response);

    if ((trackerinfo = be_decoden(r->content, r->content_length)) == NULL)
        fprintf(stderr, "Error parsing the tracker repsonse\n");

    return r;
}

/* d8:completei1e10:incompletei0e8:intervali600e5:peersld2:ip9:127.0.0.17:peer id20:b0901b3ede5354f33ec74:porti8080eeee */
void print_horizontal_line(int width){
    int i;
    printf("\t");
    for (i = 0; i < width; i++)
        printf("%s", "-");
    printf("\n");
}

void print_cell(char *value){
    int indent;

    indent = COLUMN_WIDTH - strlen(value);
    printf("%s%*s | ", value, indent, "");
}

void print_header_row(){
    int i;
    int row_width = (COLUMN_WIDTH+3) * n_announce_keys;

    printf("\t");
    for (i = 0; i < n_announce_keys; i++){
        print_cell(announce_keys[i]);
    }
    printf("\n");

    print_horizontal_line(row_width);
}

void print_info_row(){
    int row_width = (COLUMN_WIDTH+3) * n_announce_keys;
    int i, indent;

    printf("\t");
    for (i = 0; i < n_announce_keys; i++){
        indent = COLUMN_WIDTH - strlen(announce_keys[i]);
        printf("%s%*s | ", announce_keys[i], indent, "");
    }
    printf("\n");

    print_horizontal_line(row_width);
}



void print_announce(){
    http_response_t *r;
    r = announce();

    printf("\tTracker responded: %s\n", r->status_line);
    print_header_row();

    free_http_response(r);
}
