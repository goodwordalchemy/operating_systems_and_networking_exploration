#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>

#define URL_REGEX  "http[s]?://([A-Za-z0-9~\\-_.]+):?([0-9]+)?/*.*"

void extract_hostname_and_port(char *ip, char *port, char *url){
    regex_t regex;
    regmatch_t rm[3];

    if (regcomp(&regex, URL_REGEX, REG_EXTENDED))
        perror("regcomp");

    if (regexec(&regex, url, 3, rm, REG_EXTENDED))
        perror("regexec");

	sprintf(ip, "%.*s", (int)(rm[1].rm_eo - rm[1].rm_so), url + rm[1].rm_so);
	sprintf(port, "%.*s", (int)(rm[2].rm_eo - rm[2].rm_so), url + rm[2].rm_so);

    
    if (rm[2].rm_so == rm[2].rm_eo) {
        printf("port is null!\n");
        sprintf(port, "%d", 80);
    }

        
}
