#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>

#include "http_response_parse.h"

#define HTTP_RESPONSE_REGEX "(HTTP/1.1 [0-9][0-9][0-9] OK)(\r\n.*)*Content-Length: ([0-9]+)\r\n\r\n(.*)\r\n"

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

    r = malloc(sizeof(http_response_t));

    if ((rc = regcomp(&regex, HTTP_RESPONSE_REGEX, REG_EXTENDED)) != 0){
        regerror(rc, &regex, err_buf, 100);
        fprintf(stderr, "regcomp() failed with '%s'\n", err_buf);
    }

    if (regexec(&regex, response, 5, rm, 0)){
        regerror(rc, &regex, err_buf, 100);
        fprintf(stderr, "regexec() failed with '%s'\n", err_buf);
    }

    r->status_line = malloc(sizeof(char) * ((rm[1].rm_eo - rm[1].rm_so) + 1));
	sprintf(r->status_line, "%.*s", (int)(rm[1].rm_eo - rm[1].rm_so), response + rm[1].rm_so);
    sscanf(r->status_line, "HTTP/1.1 %3d%*s", &r->status_code);

	sprintf(cl_buf, "%.*s", (int)(rm[3].rm_eo - rm[3].rm_so), response + rm[3].rm_so);
    r->content_length = atoi(cl_buf);

    r->content = malloc(sizeof(char) * (r->content_length + 1));
	sprintf(r->content, "%.*s", (int)(rm[4].rm_eo - rm[4].rm_so), response + rm[4].rm_so);

    regfree(&regex);

    return r;
}

/* int main(int argc, char *argv[]) { */
/* char *response = "HTTP/1.1 200 OK\r\nDate: Fri, 01 Feb 2019 20:57:30 GMT\r\nConnection: keep-alive\r\nContent-Length: 116\r\n\r\nd8:completei1e10:incompletei0e8:intervali600e5:peersld2:ip9:127.0.0.17:peer id20:b0901b3ede5354f33ec74:porti8080eeee\r\n"; */
/*     http_response_t *content; */
/*     content = extract_response_content(response); */
/*     printf("status: %s\n", content->status_line); */
/*     printf("content:%s\n", content->content); */
/*     free_http_response(content); */
/* } */
