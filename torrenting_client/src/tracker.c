// Creates global trackerinfo object after interacting with tracker.
// All interfacing with tracker or updating of the trackerinfo object goes here.
#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include "be_node_utils.h"
#include "logging_utils.h"
#include "state.h"
#include "socket_helpers.h"

#define COMPACT_PEER_FORMAT 0

#define DATASIZE_BUFLEN 11 
#define EVENT_BUFLEN 10
#define REQUEST_BUFLEN 255
#define RESPONSE_BUFLEN 8192

#define HTTP_RESPONSE_REGEX "(HTTP/1.1 [0-9][0-9][0-9] OK)(\r\n.*)*Content-Length: ([0-9]+)\r\n\r\n(.*)"

typedef struct __http_response_t {
    char *status_line;
    int status_code;
    char *content;
    int content_length;
} http_response_t;


char *announce_keys[] = {"complete", "incomplete", "interval", "min interval"};
int n_announce_keys = 4;

char *peers_keys[] = {"peer id", "ip", "port"};
int n_peers_keys = 3;

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


void free_http_response(http_response_t *resp){
    free(resp->status_line);
    free(resp->content);
    free(resp);
}

http_response_t *extract_response_content(char *response){
    http_response_t *r;
    int rc;
    char cl_buf[6], err_buf[100];
    regex_t regex;
    regmatch_t rm[5];

    if ((r = malloc(sizeof(http_response_t))) == NULL)
        perror("http_response malloc");

    if ((rc = regcomp(&regex, HTTP_RESPONSE_REGEX, REG_EXTENDED)) != 0){
        regerror(rc, &regex, err_buf, 100);
        fprintf(stderr, "regcomp() failed with '%s'\n", err_buf);
    }

    if (regexec(&regex, response, 5, rm, 0)){
        regerror(rc, &regex, err_buf, 100);
        fprintf(stderr, "regexec() failed with '%s'\n", err_buf);
    }


    if ((r->status_line = malloc(sizeof(char) * ((rm[1].rm_eo - rm[1].rm_so) + 1))) == NULL)
        perror("status line malloc");

	sprintf(r->status_line, "%.*s", (int)(rm[1].rm_eo - rm[1].rm_so), response + rm[1].rm_so);
    sscanf(r->status_line, "HTTP/1.1 %3d%*s", &r->status_code);

	sprintf(cl_buf, "%.*s", (int)(rm[3].rm_eo - rm[3].rm_so), response + rm[3].rm_so);
    r->content_length = atoi(cl_buf);

    if ((r->content = malloc(sizeof(char) * (r->content_length + 1))) == NULL)
        perror("content malloc");

	sprintf(r->content, "%.*s", (int)(rm[4].rm_eo - rm[4].rm_so), response + rm[4].rm_so);

    regfree(&regex);

    return r;
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

void print_info_header_row(){
    int i;

    printf("\t");
    for (i = 0; i < n_announce_keys; i++){
        print_str_cell(announce_keys[i]);
    }
    printf("\n");
}


void print_info_row(){
    int val, i;

    printf("\t");
    for (i = 0; i < n_announce_keys; i++){
        if ((val = get_be_node_int(trackerinfo, announce_keys[i])) != -1)
            print_int_cell(val);
        else
            print_str_cell("-");
    }
    printf("\n");
}

/* d8:completei1e10:incompletei0e8:intervali600e5:peersld2:ip9:127.0.0.17:peer id20:b0901b3ede5354f33ec74:porti8080eeee */
void print_peers_header_row(){
    int i;

    printf("\t");
    for (i = 0; i < n_peers_keys; i++){
        print_str_cell(peers_keys[i]);
    }
    printf("\n");
}

be_node **get_peers_list(){
    return trackerinfo->val.d[3].val->val.l;
}

void print_peers_row(){
    be_node **pl;

    pl = get_peers_list();

    while (*pl){
        printf("\t");
        print_str_cell(get_be_node_str(*pl, "peer id"));
        print_str_cell(get_be_node_str(*pl, "ip"));
        print_int_cell(get_be_node_int(*pl, "port"));
        pl++;
        printf("\n");
    }
}

void print_trackerinfo(){
    int info_row_width = (COLUMN_WIDTH+3) * n_announce_keys;
    int peers_row_width = (COLUMN_WIDTH+3) * n_peers_keys;

    if (trackerinfo == NULL){
        fprintf(stderr, "Error: Need to announce yourself to tracker first\n");
        return;
    }

    printf("\tTracker info\n\n");
    print_info_header_row();
    print_horizontal_line(info_row_width);
    print_info_row();
    print_horizontal_line(info_row_width);

    printf("\tPeer List:\n\n");
    print_peers_header_row();
    print_horizontal_line(peers_row_width);
    print_peers_row();
    print_horizontal_line(peers_row_width);
}

void print_announce(){
    http_response_t *r;

    r = announce();
    printf("\tTracker responded: %s\n", r->status_line);
    free_http_response(r);

    print_trackerinfo();
}
